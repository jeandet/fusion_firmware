LIBRARY ieee;
USE ieee.std_logic_1164.ALL;
USE ieee.numeric_std.ALL;

ENTITY top_sticky IS
    GENERIC (
        PHY_CLK_PERIOD : TIME := 20.833333 ns;
        SMP_CLK_PERIOD : TIME := 310 ns;
        CLK_DIV : INTEGER := 1;
        CLK_MULT : INTEGER := 2
    );
    PORT (
        reset : IN STD_LOGIC;
        led : OUT STD_LOGIC
    );
END ENTITY;

ARCHITECTURE ar_top_sticky OF top_sticky IS
    COMPONENT clock IS
        GENERIC (
            CLK_MUL : INTEGER := 1;
            CLK_DIV : INTEGER := 2
        );
        PORT (
            clk_in : IN STD_LOGIC;
            reset : IN STD_LOGIC;
            clk : OUT STD_LOGIC
        );
    END COMPONENT;

    CONSTANT CLK_PERIOD : TIME := (REAL(CLK_DIV) / REAL(CLK_MULT)) * PHY_CLK_PERIOD;
    SIGNAL clk : STD_LOGIC;
    SIGNAL cptr : INTEGER RANGE 0 TO 200000 := 0;
    SIGNAL led_reg : STD_LOGIC := '0';

BEGIN

    clk0 : clock
    GENERIC MAP(CLK_DIV => 128)
    PORT MAP(
        clk_in => '1',
        reset => reset,
        clk => clk
    );

    led <= led_reg;
    PROCESS (clk, reset)
    BEGIN
        IF reset = '0' THEN
            cptr <= 0;
        ELSIF clk'event AND clk = '1' THEN
            IF cptr = 200000 THEN
                cptr <= 0;
                led_reg <= NOT led_reg;
            ELSE
                cptr <= cptr + 1;
            END IF;
        END IF;
    END PROCESS;

END ARCHITECTURE ar_top_sticky;