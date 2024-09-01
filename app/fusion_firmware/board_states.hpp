#pragma once

enum class BoardState
{
    Idle,
    ProgrammingFPGA,
    Running
};

extern volatile BoardState board_state;