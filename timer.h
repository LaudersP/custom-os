#pragma once

#include "utils.h"

static unsigned divisor;
static unsigned clockTicks = 0;

void timer_init(const unsigned rateDivisor);

void increase_clock_ticks();
unsigned get_rate();
unsigned get_uptime();

u16 inw(u16 port);
void outw(u16 port, u16 value);
u32 inl(u16 port);
void outl(u16 port, u32 value);
