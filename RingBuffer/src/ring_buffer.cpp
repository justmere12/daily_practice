#include "ring_buffer.h"

template class RingBuffer<int, 1024>;
template class RingBuffer<int, 4096>;
template class RingBuffer<int, 65536>;

template class RingBuffer<unsigned int, 1024>;
template class RingBuffer<unsigned int, 4096>;

template class RingBuffer<float, 1024>;
template class RingBuffer<float, 4096>;

template class RingBuffer<double, 1024>;
template class RingBuffer<double, 4096>;

template class RingBuffer<char, 1024>;
template class RingBuffer<char, 4096>;

template class RingBuffer<void*, 1024>;
template class RingBuffer<void*, 4096>;