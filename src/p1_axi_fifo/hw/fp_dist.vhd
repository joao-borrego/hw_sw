----------------------------------------------------------------------
--! @file fp_dist.vhd
--! @brief Implements a module for calculating (A-B)^2 with floats
--!
--! This module accepts two single precision float operands A and B,
--! and performs the calculation of (A-B)^2 and accumulates the result.
--! It is used to calculate square distances between data points.
----------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

--! Main module entity declaration
entity fp_dist is
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
end fp_dist;

--! Architecture declaration
architecture Behavioral of fp_dist is

    -- Component declarations

    -- Floating Point Subtractor Declaration
    COMPONENT fp_sub
        PORT (
            aclk : IN STD_LOGIC;
            s_axis_a_tvalid : IN STD_LOGIC;
            s_axis_a_tdata  : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
            s_axis_a_tlast  : IN STD_LOGIC;
            s_axis_b_tvalid : IN STD_LOGIC;
            s_axis_b_tdata  : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
            m_axis_result_tvalid    : OUT STD_LOGIC;
            m_axis_result_tdata     : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
            m_axis_result_tlast     : OUT STD_LOGIC
        );
    END COMPONENT;

    -- Floating Point Multiplier Declaration
    COMPONENT fp_mul
        PORT (
            aclk : IN STD_LOGIC;
            s_axis_a_tvalid : IN STD_LOGIC;
            s_axis_a_tdata  : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
            s_axis_a_tlast  : IN STD_LOGIC;
            s_axis_b_tvalid : IN STD_LOGIC;
            s_axis_b_tdata  : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
            m_axis_result_tvalid    : OUT STD_LOGIC;
            m_axis_result_tdata     : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
            m_axis_result_tlast     : OUT STD_LOGIC
        );
    END COMPONENT;

    -- Floating Point Accumulator Declaration
    COMPONENT fp_acc
        PORT (
            aclk : IN STD_LOGIC;
            aresetn         : IN STD_LOGIC;
            s_axis_a_tvalid : IN STD_LOGIC;
            s_axis_a_tdata  : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
            s_axis_a_tlast  : IN STD_LOGIC;
            m_axis_result_tvalid    : OUT STD_LOGIC;
            m_axis_result_tdata     : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
            m_axis_result_tlast     : OUT STD_LOGIC
        );
    END COMPONENT;

    -- Internal signal declarations

    -- Internal FP Subtractor signals
    signal sub_valid    : std_logic;
    signal sub_last     : std_logic;
    signal sub_data     : std_logic_vector(31 downto 0);

    -- Internal FP Multiplier signals
    signal mul_valid  : std_logic;
    signal mul_last   : std_logic;
    signal mul_data   : std_logic_vector(31 downto 0);

    -- Internal FP Accumulator Signals
    signal acc_reset    : std_logic;
    signal init_acc_d1  : std_logic;

    -- Internal Registers for the Input Operands
    signal reg_A : std_logic_vector(31 downto 0);
    signal reg_B : std_logic_vector(31 downto 0);
    signal reg_valid_A     : std_logic;
    signal reg_last_A      : std_logic;

begin

    -- Instantiation of pre-made IPs

    -- Floating Point Subtractor Instantiation
    inst_fp_sub : fp_sub
        PORT MAP (
            aclk => clk,
            s_axis_a_tvalid   => reg_valid_A,
            s_axis_a_tdata    => reg_A,
            s_axis_a_tlast    => reg_last_A,
            s_axis_b_tvalid   => reg_valid_A,
            s_axis_b_tdata    => data_B,
            m_axis_result_tvalid  => sub_valid,
            m_axis_result_tdata   => sub_data,
            m_axis_result_tlast   => sub_last
        );

    -- Floating Point Multiplier Instantiation
    inst_fp_mul : fp_mul
        PORT MAP (
            aclk => clk,
            s_axis_a_tvalid => sub_valid,
            s_axis_a_tdata  => sub_data,
            s_axis_a_tlast  => sub_last,
            s_axis_b_tvalid => sub_valid,
            s_axis_b_tdata  => sub_data,
            m_axis_result_tvalid    => mul_valid,
            m_axis_result_tdata     => mul_data,
            m_axis_result_tlast     => mul_last
        );

    -- Floating Point Accumulator Instantiation
    ins_fp_acc : fp_acc
        PORT MAP (
            aclk => clk,
            aresetn => acc_reset,
            s_axis_a_tvalid => mul_valid,
            s_axis_a_tdata  => mul_data,
            s_axis_a_tlast  => mul_last,
            m_axis_result_tvalid    => valid_out,
            m_axis_result_tdata     => data_out,
            m_axis_result_tlast     => last_out
        );

    -- Accumulator - Ensure assertion for a minimum of 2 clock cycles
    acc_reset <= init_acc_d1 nor init_acc;

    ----------------------------------------------------------------------
    --! @brief Process to update the main input signals
    --! @param[in] clk Clock, used on rising edge
    ----------------------------------------------------------------------
    in_update_proc: process (clk)
        begin
            if clk'event and clk='1' then  
                init_acc_d1   <= init_acc;
                reg_A         <= data_A;
                reg_valid_A   <= valid_A;
                reg_last_A    <= last_A;
            end if;
    end process;

end Behavioral;
