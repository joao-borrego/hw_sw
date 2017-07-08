----------------------------------------------------------------------
--! @file tb_my_fp_dist_v3_0.vhd
--! @brief Testbench for my_fp_dist_v3_0 Module
--!
--! Calculates the squared distance matrix from a feature matrix of
--! training examples to a feature matrix of testing examples.
----------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

library ieee_proposed;
use ieee_proposed.float_pkg.all;

entity tb_fp_dist is
    --  Port ( );
end tb_fp_dist;

--! Behavioral architecture description of UUT
architecture Behavioral of tb_fp_dist is

    --! UUT component declaration
    component my_fp_dist_v4_0
        generic (
            C_S_AXIS_TDATA_WIDTH    : integer    := 32;
            C_M_AXIS_TDATA_WIDTH    : integer    := 32;
            C_M_AXIS_START_COUNT    : integer    := 32);
        port (
            -- Ports of Axi Slave Bus Interface S_AXIS
            s_axis_aclk     : in std_logic;
            s_axis_aresetn  : in std_logic;
            s_axis_tready   : out std_logic;
            s_axis_tdata    : in std_logic_vector(C_S_AXIS_TDATA_WIDTH-1 downto 0);
            s_axis_tstrb    : in std_logic_vector((C_S_AXIS_TDATA_WIDTH/8)-1 downto 0);
            s_axis_tlast    : in std_logic;
            s_axis_tvalid   : in std_logic;
            -- Ports of Axi Master Bus Interface M_AXIS
            m_axis_aclk     : in std_logic;
            m_axis_aresetn  : in std_logic;
            m_axis_tvalid   : out std_logic;
            m_axis_tdata    : out std_logic_vector(C_M_AXIS_TDATA_WIDTH-1 downto 0);
            m_axis_tstrb    : out std_logic_vector((C_M_AXIS_TDATA_WIDTH/8)-1 downto 0);
            m_axis_tlast    : out std_logic;
            m_axis_tready   : in std_logic );
    end component;

    -- Inputs
    signal clk : std_logic := '0';
    signal rstn : std_logic := '0';

    signal S_AXIS_TDATA : std_logic_vector(31 downto 0) := (others => '0');
    signal S_AXIS_TSTRB : std_logic_vector(3 downto 0) := (others => '1');
    signal S_AXIS_TLAST : std_logic := '0';
    signal S_AXIS_TVALID : std_logic := '0';
    signal M_AXIS_TREADY : std_logic := '0';

    -- Outputs
    signal M_AXIS_TDATA : std_logic_vector(31 downto 0);
    signal M_AXIS_TSTRB : std_logic_vector(3 downto 0);
    signal M_AXIS_TLAST : std_logic;
    signal M_AXIS_TVALID : std_logic;
    signal S_AXIS_TREADY : std_logic;

    -- Clock period definitions
    constant clk_period : time := 10 ns;
    
    -- Whether B is read to S_AXIS_TDATA
    signal selB : std_logic := '0';
    
    -- Terminator
    constant TERMINATOR : real := 1.0E30;
    
    -- Input data
    constant TRN_SIZE : integer := 3;
    constant TST_SIZE : integer := 4;
    constant FEATURES : integer := 4;
    
    signal rdataA : real := 0.0;
    signal rdataB : real := 0.0;
    type trn_array is array (0 to 11) of real;
    type tst_array is array (0 to 15) of real;
    
    -- Testing Objcts
    -- Each line corresponds to a testing example
    -- Each column corresponds to one of the 4 features
    signal ra : trn_array := (
        1.5,    2.5,    3.5,    4.5, 
        5.5,    -6.0,   7.5,    8.5, 
        0.0,     0.0,   0.0,    0.0
    );

    -- Training Objects
    signal rb : tst_array := (
         1.0,  2.0,  3.0,  4.0,
         5.0,  6.0,  7.0,  8.0,
         9.0, 10.0, 11.0, 12.0,
        13.0, 14.0, 15.0, 16.0
    );

begin

    -- Instantiate the Unit Under Test (UUT)
    uut: my_fp_dist_v4_0
    PORT MAP (
        S_AXIS_ACLK     => clk,
        S_AXIS_ARESETN  => rstn,
        S_AXIS_TREADY   => S_AXIS_TREADY,
        S_AXIS_TDATA    => S_AXIS_TDATA,
        S_AXIS_TSTRB    => S_AXIS_TSTRB,
        S_AXIS_TLAST    => S_AXIS_TLAST,
        S_AXIS_TVALID   => S_AXIS_TVALID,
        M_AXIS_ACLK     => clk,
        M_AXIS_ARESETN  => rstn,
        M_AXIS_TVALID   => M_AXIS_TVALID,
        M_AXIS_TDATA    => M_AXIS_TDATA,
        M_AXIS_TSTRB    => M_AXIS_TSTRB,
        M_AXIS_TLAST    => M_AXIS_TLAST,
        M_AXIS_TREADY   => M_AXIS_TREADY
    );

    -- Clock definition
    clk <= not clk after clk_period/2;

    S_AXIS_TDATA <= to_slv( to_float(rdataB)) when selB='1' else
                    to_slv( to_float(rdataA));

    ----------------------------------------------------------------------
    --! @brief Stimulus Process
    ----------------------------------------------------------------------
    stim_proc: process
    begin

        -- Hold reset state for 100 ns.
        wait for 100 ns;

        rstn <= '1';

        wait for 96 ns; -- to synchronize input variation with positive clock edge + 1ns
    
        -- For each testing object
        for i in 0 to TST_SIZE - 1 loop
            
            -- Copy each coordinate of tst object
            for j in 0 to FEATURES - 1 loop
                -- wait for clk_period*10;
                selB <= '1';
                rdataB <= rb(i*4 + j);   -- B is sent by lines
            
                if j = FEATURES - 1 then
                    S_AXIS_TLAST <= '1';
                else
                    S_AXIS_TLAST <= '0';
                end if;
                S_AXIS_TVALID <= '1';
            
                wait for clk_period;
                S_AXIS_TVALID <= '0';
                S_AXIS_TLAST <= '0';
            
            end loop;
            
            -- For each training object
            for k in 0 to TRN_SIZE -1 loop
                
                if (k = TRN_SIZE - 1) then
                    -- Send terminator
                    S_AXIS_TVALID <= '1';
                    S_AXIS_TLAST <= '1';
                    rdataA <= TERMINATOR;
                    wait for clk_period;
                    S_AXIS_TVALID <= '0';
                    S_AXIS_TLAST <= '0';
                end if;
                
                wait for clk_period;
                
                -- Copy each coordinate of trn object
                for j in 0 to FEATURES - 1 loop
                    
                    selB <= '0';
                    rdataA <= ra(k*4+j);

                    if j = FEATURES - 1 then
                        S_AXIS_TLAST <= '1';
                    else
                        S_AXIS_TLAST <= '0';
                    end if;
                       
                    S_AXIS_TVALID <= '1';

                    wait for clk_period;
                    S_AXIS_TVALID <= '0';
                    S_AXIS_TLAST <= '0';

                end loop;

                wait until M_AXIS_TVALID='1';
                M_AXIS_TREADY <= '1';

                wait for clk_period;
                M_AXIS_TREADY <= '0';

            end loop;
            
            wait for clk_period*10;       
            
        end loop;
        wait;
 
    end process;

end Behavioral;
