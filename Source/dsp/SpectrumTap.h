#pragma once
#include <array>
#include <atomic>
#include <cstddef>

namespace blockade
{
    // Hands the mono input and output signals from the audio thread to the editor.
    // The audio thread writes into a ring; the editor copies out the newest samples.
    // The ring is far longer than any read, so the writer never laps the reader.
    class SpectrumTap
    {
    public:
        static constexpr size_t capacity = 1 << 14;

        void push(float pre, float post)
        {
            const size_t i = head.load(std::memory_order_relaxed);
            preRing[i & mask] = pre;
            postRing[i & mask] = post;
            head.store(i + 1, std::memory_order_release);
        }

        // Copies the newest `count` samples of both signals, oldest first.
        void latest(float* pre, float* post, size_t count) const
        {
            const size_t start = head.load(std::memory_order_acquire) - count;
            for (size_t k = 0; k < count; ++k)
            {
                pre[k] = preRing[(start + k) & mask];
                post[k] = postRing[(start + k) & mask];
            }
        }

    private:
        static constexpr size_t mask = capacity - 1;

        std::array<float, capacity> preRing {}, postRing {};
        std::atomic<size_t> head { 0 };
    };
}
