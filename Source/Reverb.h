#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Reverb algorithmique originale : FDN (Feedback Delay Network) 8 lignes,
// matrice de Hadamard, amortissement des aigus, légère modulation, pré-delay.
class EarthReverb
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const int maxLine = (int) std::ceil (sr * 0.072 * 3.3) + 64;
        for (auto& l : lines) l.prepare (maxLine);
        const int maxPre = (int) std::ceil (sr * 0.5) + 8;
        preL.prepare (maxPre);
        preR.prepare (maxPre);

        static const float baseMs[8] = { 29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.9f, 67.7f, 71.9f };
        static const float lfoHz[8]  = { 0.11f, 0.17f, 0.23f, 0.29f, 0.13f, 0.19f, 0.31f, 0.37f };
        for (int i = 0; i < 8; ++i)
        {
            baseSamples[i] = baseMs[i] * (float) sr / 1000.0f;
            lfoInc[i]      = lfoHz[i] / (float) sr;
            phase[i]       = (float) i / 8.0f;
        }
        modDepth = 3.0f * (float) (sr / 44100.0);
        reset();
        setParams (2.5f, 0.0f, 0.5f, 0.4f);
        curSize = sizeTarget;
        curPre  = preTarget;
    }

    void reset()
    {
        for (auto& l : lines) l.clear();
        preL.clear(); preR.clear();
        for (auto& x : lp) x = 0.0f;
    }

    // timeSec : durée de queue (RT60) | preDelayMs | size01 : taille | decay01 : absorption des aigus
    void setParams (float timeSec, float preDelayMs, float size01, float decay01)
    {
        const float rt60 = std::max (0.05f, timeSec);
        sizeTarget = 0.35f + size01 * 1.65f;
        preTarget  = std::max (1.0f, preDelayMs * (float) sr / 1000.0f);

        const float fc = 16000.0f * std::pow (0.12f, decay01);       // 16 kHz -> ~1.9 kHz
        dampA = 1.0f - std::exp (-6.2831853f * fc / (float) sr);

        for (int i = 0; i < 8; ++i)
        {
            const float lenSec = baseSamples[i] * sizeTarget / (float) sr;
            g[i] = std::pow (10.0f, -3.0f * lenSec / rt60);
        }
    }

    void process (float inL, float inR, float& outL, float& outR)
    {
        curSize += (sizeTarget - curSize) * 0.0004f;
        curPre  += (preTarget  - curPre)  * 0.0004f;

        preL.push (inL);
        preR.push (inR);
        const float pl = preL.read (std::max (1.0f, curPre));
        const float pr = preR.read (std::max (1.0f, curPre));

        float v[8], d[8];
        for (int i = 0; i < 8; ++i)
        {
            phase[i] += lfoInc[i];
            if (phase[i] >= 1.0f) phase[i] -= 1.0f;
            const float len = baseSamples[i] * curSize + modDepth * std::sin (6.2831853f * phase[i]);
            d[i]   = lines[i].read (std::max (1.0f, len));
            lp[i] += dampA * (d[i] - lp[i]);
            v[i]   = lp[i] * g[i];
        }

        // Matrice de Hadamard 8x8 (orthonormale => stable)
        for (int h = 1; h < 8; h *= 2)
            for (int i = 0; i < 8; i += 2 * h)
                for (int j = i; j < i + h; ++j)
                {
                    const float x = v[j], y = v[j + h];
                    v[j] = x + y;
                    v[j + h] = x - y;
                }

        const float norm = 0.35355339f;
        for (int i = 0; i < 8; ++i)
            lines[i].push (v[i] * norm + ((i & 1) ? pr : pl) * 0.5f);

        outL = (lp[0] - lp[2] + lp[4] - lp[6]) * 0.5f;
        outR = (lp[1] - lp[3] + lp[5] - lp[7]) * 0.5f;
    }

private:
    struct DelayLine
    {
        std::vector<float> buf; int w = 0; int size = 1;
        void prepare (int n) { size = n; buf.assign ((size_t) n, 0.0f); w = 0; }
        void clear() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
        void push (float x) { buf[(size_t) w] = x; if (++w >= size) w = 0; }
        float read (float d) const
        {
            float r = (float) w - d;
            while (r < 0.0f) r += (float) size;
            int i0 = (int) r; float f = r - (float) i0;
            if (i0 >= size) i0 -= size;
            int i1 = i0 + 1; if (i1 >= size) i1 = 0;
            return buf[(size_t) i0] * (1.0f - f) + buf[(size_t) i1] * f;
        }
    };

    DelayLine lines[8], preL, preR;
    double sr = 44100.0;
    float baseSamples[8] {}, lfoInc[8] {}, phase[8] {}, lp[8] {}, g[8] {};
    float dampA = 0.5f, modDepth = 3.0f;
    float sizeTarget = 1.0f, curSize = 1.0f, preTarget = 1.0f, curPre = 1.0f;
};
