----------------------------------------------------------------------
--! @file my_fp_dist_dma_v1_0.vhd
--! @brief AXI Stream Calculation of squared euclidean distance w/ DMA
--!
--! Module for calculating squared euclidean distance between training
--! and testing examples, with single-precision float feature vectors.
--! Has AXI Stream interfaces for I/O. 
--! Improved state machine with terminator sequence
--! 0x7149f2ca or 1.0E30 
----------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

--! Main module entity declaration
entity my_fp_dist_dma_v1_0 is
	generic (
		-- Users to add parameters here

		-- User parameters ends
		-- Do not modify the parameters beyond this line

		-- Parameters of Axi Slave Bus Interface S_AXIS
		C_S_AXIS_TDATA_WIDTH	: integer	:= 32;

		-- Parameters of Axi Master Bus Interface M_AXIS
		C_M_AXIS_TDATA_WIDTH	: integer	:= 32;
		C_M_AXIS_START_COUNT	: integer	:= 32
	);
	port (
		-- Users to add ports here
		
		-- User ports ends
		-- Do not modify the ports beyond this line

		-- Ports of Axi Slave Bus Interface S_AXIS
		s_axis_aclk       : in std_logic;
		s_axis_aresetn    : in std_logic;
		s_axis_tready     : out std_logic;
		s_axis_tdata      : in std_logic_vector(C_S_AXIS_TDATA_WIDTH-1 downto 0);
		s_axis_tstrb      : in std_logic_vector((C_S_AXIS_TDATA_WIDTH/8)-1 downto 0);
		s_axis_tlast      : in std_logic;
		s_axis_tvalid     : in std_logic;

		-- Ports of Axi Master Bus Interface M_AXIS
		m_axis_aclk       : in std_logic;
		m_axis_aresetn    : in std_logic;
		m_axis_tvalid     : out std_logic;
		m_axis_tdata      : out std_logic_vector(C_M_AXIS_TDATA_WIDTH-1 downto 0);
		m_axis_tstrb      : out std_logic_vector((C_M_AXIS_TDATA_WIDTH/8)-1 downto 0);
		m_axis_tlast      : out std_logic;
		m_axis_tready     : in std_logic
	);
end my_fp_dist_dma_v1_0;

--! Architecture Declaration 
architecture arch_imp of my_fp_dist_dma_v1_0 is

	-- Component declaration

	-- AXI Stream Slave Interface Declaration
	component my_fp_dist_dma_v1_0_S_AXIS is
        generic (
            C_S_AXIS_TDATA_WIDTH	: integer	:= 32
		);
        port (
            S_AXIS_ACLK	    : in std_logic;
            S_AXIS_ARESETN	: in std_logic;
            S_AXIS_TREADY	: out std_logic;
            S_AXIS_TDATA	: in std_logic_vector(C_S_AXIS_TDATA_WIDTH-1 downto 0);
            S_AXIS_TSTRB	: in std_logic_vector((C_S_AXIS_TDATA_WIDTH/8)-1 downto 0);
            S_AXIS_TLAST	: in std_logic;
            S_AXIS_TVALID	: in std_logic
        );
	end component my_fp_dist_dma_v1_0_S_AXIS;

    -- AXI Stream Master Interface Declaration
	component my_fp_dist_dma_v1_0_M_AXIS is
        generic (
            C_M_AXIS_TDATA_WIDTH	: integer	:= 32;
            C_M_START_COUNT         : integer	:= 32
        );
        port (
            M_AXIS_ACLK     : in std_logic;
            M_AXIS_ARESETN	: in std_logic;
            M_AXIS_TVALID	: out std_logic;
            M_AXIS_TDATA	: out std_logic_vector(C_M_AXIS_TDATA_WIDTH-1 downto 0);
            M_AXIS_TSTRB	: out std_logic_vector((C_M_AXIS_TDATA_WIDTH/8)-1 downto 0);
            M_AXIS_TLAST	: out std_logic;
            M_AXIS_TREADY	: in std_logic
        );
	end component my_fp_dist_dma_v1_0_M_AXIS;

    -- Signal definitions

    -- Internal signals for 10 bit Read/Write counters
    signal rdcount, wrcount, vect_size : unsigned(9 downto 0);
    signal rst_rdcount, rst_rdcnt, en_rdcount, store_size : std_logic;
    signal rst_wrcount, rst_wrcnt, en_wrcount, last_write, last_acc : std_logic;
    signal last_distance, prev_last_distance : std_logic;
	signal last_feat : std_logic;
    
    -- Indicate whether input A is the last until output is ready, and whether A is valid
    signal a_last, a_valid : std_logic;

    -- Signals for managing read/write permission
    signal f_can_write, f_can_read, last_col_elem : std_logic;

    -- BRAM memory internal signals
    signal mem_out : std_logic_vector(31 downto 0);
    signal mem_wr_en : std_logic_vector(0 downto 0);

    -- 3-State Finite State Machine (FSM) signal definitions
    -- st_read_B Read array of operands B to BRAM, so they can be reused
    -- st_read_A Read an instance of operand A
    -- st_write Write result to output 
    type state_type is (st_read_B, st_read_A, st_write);
    signal state, next_state : state_type;

    -- Component declaration

    -- BRAM memory Declaration
    COMPONENT blk_mem_gen_0
        PORT (
            clka    : IN STD_LOGIC;
            wea     : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
            addra   : IN STD_LOGIC_VECTOR(9 DOWNTO 0);
            dina    : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
            douta   : OUT STD_LOGIC_VECTOR(31 DOWNTO 0)
        );
    END COMPONENT;

    -- FP Distance (A-B)^2 Module Declaration
    component fp_dist is
        generic (DATA_SIZE : natural := 32);
        port (
            data_A, data_B : in  std_logic_vector (DATA_SIZE-1 downto 0); --! Input
            clk:        in std_logic;   --! Clock
            init_acc:   in std_logic;   --! Initialise Accumulator
            valid_A:    in std_logic;   --! Value A in reg_A is valid 
            last_A :    in std_logic;   --! Last operation; afterwards, accumulator is reset
            valid_out:  out std_logic;  --! Output is valid
            last_out :  out std_logic;  --! Last operation performed; Output is ready
            data_out :  out  std_logic_vector (DATA_SIZE-1 downto 0) --! Output
        );
    end component;

begin

    -- Control Unit internal connections
    M_AXIS_TVALID <= f_can_write;
    S_AXIS_TREADY <= f_can_read;
    M_AXIS_TLAST <= last_col_elem;
    M_AXIS_TSTRB <= "1111";

    ----------------------------------------------------------------------
    --! @brief Process to manage the state machine registers
    --! @param[in] S_AXIS_ACLK Clock, used on rising edge
    ----------------------------------------------------------------------
    FSM_state_reg: process (S_AXIS_ACLK)
    begin
        if (S_AXIS_ACLK'event and S_AXIS_ACLK = '1') then
            if (S_AXIS_ARESETN='0') then
                state <= st_read_B;
            else
                state <= next_state;
            end if;
        end if;
    end process;

    ----------------------------------------------------------------------
    --! @brief Process to manage the state machine transitions
    --! @param[in] state Current state
    --! @param[in] S_AXIS_TVALID
    --! @param[in] S_AXIS_TLAST
    --! @param[in] M_AXIS_TREADY
    --! @param[in] last_acc Last operation to accumulate 
    --! @param[in] last_write Last write operation
    ----------------------------------------------------------------------
    FSM_comb_logic: process (
        state,
        S_AXIS_TVALID,
        S_AXIS_TLAST,
        S_AXIS_TDATA,
        M_AXIS_TREADY,
        last_acc,
        last_write,
        last_distance,
        prev_last_distance,
        last_feat
    )
    begin

        -- declare default values (0) to avoid latches
        next_state  <= state;  -- default is to stay in current state
        f_can_write     <= '0';
        f_can_read      <= '0';
        mem_wr_en(0)    <= '0';
        store_size      <= '0';
        a_last          <= '0';
        a_valid         <= '0';
        en_wrcount      <= '0';
        rst_wrcount     <= '0'; 
        en_rdcount      <= '0';
        rst_rdcount     <= '0'; 
        last_col_elem   <= '0';
        last_distance   <= prev_last_distance;

        case (state) is
            
            when st_read_B =>
                
                last_distance <= '0';
                
                if (S_AXIS_TVALID = '1') then
                    f_can_read <= '1';
                    en_rdcount <= '1';
                    mem_wr_en(0) <= '1';
                    if (S_AXIS_TLAST = '1') then
                        next_state <= st_read_A;
                        rst_rdcount <= '1'; 
                        store_size <= '1'; 
                        rst_wrcount <= '1'; 
                    end if;
                end if;
            
            when st_read_A =>

                if (S_AXIS_TVALID = '1') then
                    f_can_read <= '1';
                    en_rdcount <= '1';
                    a_valid <= '1';
                    if (S_AXIS_TLAST = '1') then
						last_distance <= '1';
                    end if;
                    if(last_feat = '1') then 
                        a_last <= '1';
						next_state <= st_write;
                    end if;
                    
                end if;
            
            when st_write =>

                if (last_acc = '1') then
                    f_can_write <= '1';
                    if (M_AXIS_TREADY = '1') then
                        en_wrcount <= '1';
                        rst_rdcount <= '1';
                        if (last_distance = '1') then
                            last_col_elem <= '1';    
                            next_state <= st_read_B;
                        else
                            next_state <= st_read_A;
                        end if;
                        
                    end if;
                end if;
        end case;
    end process;

    -- Logic for read and write counters
    rst_rdcnt <= rst_rdcount or (not S_AXIS_ARESETN);
    rst_wrcnt <= rst_wrcount or (not S_AXIS_ARESETN);
	last_feat <= '1' when rdcount = vect_size else '0';
    ----------------------------------------------------------------------
    --! @brief Avoid latch for last_distance signal
    --! @param[in] S_AXIS_ACLK Clock, used on rising edge
    ----------------------------------------------------------------------
    avoid_latch_distance: process (S_AXIS_ACLK) 
    begin
        if S_AXIS_ACLK='1' and S_AXIS_ACLK'event then
            prev_last_distance <= last_distance;
        end if;
    end process;

    ----------------------------------------------------------------------
    --! @brief Process to count how many frames are read
    --! @param[in] S_AXIS_ACLK Clock, used on rising edge
    ----------------------------------------------------------------------
    rd_counter_proc: process (S_AXIS_ACLK) 
    begin
        if S_AXIS_ACLK='1' and S_AXIS_ACLK'event then
            if rst_rdcnt='1' then 
                rdcount <= (others => '0');
            elsif en_rdcount='1' then
                rdcount <= rdcount + 1;
            end if;
        end if;
    end process;

    ----------------------------------------------------------------------
    --! @brief Process to count how many frames are written
    --! @param[in] S_AXIS_ACLK Clock, used on rising edge
    ----------------------------------------------------------------------
    wr_counter_proc: process (S_AXIS_ACLK) 
    begin
        if S_AXIS_ACLK='1' and S_AXIS_ACLK'event then
            if rst_wrcnt='1' then 
                wrcount <= (others => '0');
            elsif en_wrcount='1' then
                wrcount <= wrcount + 1;
            end if;
        end if;
    end process;


	-- Column Size Register
	process (S_AXIS_ACLK) 
	begin
		if S_AXIS_ACLK='1' and S_AXIS_ACLK'event then
		  if store_size='1' then
			vect_size <= rdcount;
		  end if;
		end if;
	end process;

    -- BRAM memory Instantiation
    inst_mem: blk_mem_gen_0
        port map (
            clka     => S_AXIS_ACLK,
            wea      => mem_wr_en,
            addra    => std_logic_vector(rdcount),
            dina     => S_AXIS_TDATA,
            douta    => mem_out
        );

    -- FP Distance (A-B)^2 Module Instantiation
    inst_fp_dist: fp_dist 
        generic map (
            DATA_SIZE => C_S_AXIS_TDATA_WIDTH
        )
        port map (
            data_A       => S_AXIS_TDATA,
            data_B       => mem_out,
            clk          => S_AXIS_ACLK,
            init_acc     => rst_rdcount,
            valid_A      => a_valid,
            last_A       => a_last,
            data_out     => M_AXIS_TDATA,
            valid_out    => open, 
            last_out     => last_acc
        );    

end arch_imp;