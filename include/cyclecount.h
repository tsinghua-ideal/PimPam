/*
MIT License

Copyright (c) 2021 SAFARI Research Group at ETH Zürich

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <perfcounter.h>

// Timer
typedef struct perfcounter_cycles {
    perfcounter_t start;
    perfcounter_t end;
    perfcounter_t end2;

}perfcounter_cycles;

void timer_start(perfcounter_cycles *cycles) {
    cycles->start = perfcounter_get(); // START TIMER
}

uint64_t timer_stop(perfcounter_cycles *cycles) {
    cycles->end = perfcounter_get(); // STOP TIMER
    cycles->end2 = perfcounter_get(); // STOP TIMER
    return(((uint64_t)((uint32_t)(((cycles->end >> 4) - (cycles->start >> 4)) - ((cycles->end2 >> 4) - (cycles->end >> 4))))) << 4);
}

