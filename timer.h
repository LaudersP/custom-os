#pragma once

#include "utils.h"

static unsigned divisor;
static unsigned clockTicks = 0;

void timer_init(const unsigned rateDivisor);

void increase_clock_ticks();
unsigned get_rate();
unsigned get_uptime();
