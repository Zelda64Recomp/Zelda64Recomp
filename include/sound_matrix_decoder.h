#pragma once

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <tuple>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * Passive matrix decoder for stereo to 5.1 surround upmixing.
 * Implements standard audio matrix decoding techniques using:
 * - Linkwitz-Riley crossover filters for frequency band separation
 * - All-pass filters for phase manipulation
 * - Delay lines for surround channel timing
 */
class SoundMatrixDecoder {
  public:
    /**
     * Construct and initialize the decoder with a specific sample rate.
     * @param sampleRate The audio sample rate in Hz
     */
    SoundMatrixDecoder(int32_t sampleRate);
    ~SoundMatrixDecoder() = default;

    /**
     * Reset filter states without recomputing coefficients.
     * Useful when audio is interrupted to prevent clicks.
     */
    void ResetState();

    /**
     * Decode stereo to 5.1 surround
     * @param stereoInput Interleaved stereo samples [L0, R0, L1, R1, ...]
     * @param samplePairs Number of stereo sample pairs to process
     * @return Pointer to internal buffer with interleaved 5.1 samples [FL, FR, C, LFE, SL, SR, ...]
     */
    std::tuple<const float*, size_t> Process(const float* stereoInput, size_t samplePairs);

  private:
    // 4th-order IIR filter (Linkwitz-Riley) for 24dB/octave slopes
    struct BiquadCascade {
        double X[4] = {}; // Input history
        double Y[4] = {}; // Output history
    };

    struct FilterCoefficients {
        double A[5]; // Feedforward (numerator)
        double B[4]; // Feedback (denominator, excluding b0=1)
    };

    // Sweeping all-pass for phase decorrelation
    struct AllPassChain {
        double Freq = 0;
        double FreqMin = 0;
        double FreqMax = 0;
        double SweepRate = 0;
        double XHist[4] = {};
        double YHist[4] = {};
        bool Ready = false;
    };

    // Circular delay buffer
    static constexpr int gMaxDelay = 1024;
    struct CircularDelay {
        std::array<float, gMaxDelay> Data = {};
        int Head = 0;
        int Length = 0;
    };

    // Filter design
    FilterCoefficients DesignLowPass(double frequency, int32_t sampleRate);
    FilterCoefficients DesignHighPass(double frequency, int32_t sampleRate);

    // Signal processing
    float ProcessFilter(float sample, BiquadCascade& state, const FilterCoefficients& coef);
    void PrepareAllPass(AllPassChain& chain, int32_t sampleRate);
    float ProcessAllPass(float sample, AllPassChain& chain, bool negate);
    float ProcessDelay(float sample, CircularDelay& buffer);

    static float Saturate(float value);

    int32_t mDelayLength = 0;
    double mAllPassBaseRate = 1.0; // Precomputed for ProcessAllPass

    // Filter coefficients (computed once per sample rate)
    FilterCoefficients mCoefCenterHP;
    FilterCoefficients mCoefCenterLP;
    FilterCoefficients mCoefSurroundHP;
    FilterCoefficients mCoefSubLP;

    // Per-channel filter states
    BiquadCascade mCenterHighPass;
    BiquadCascade mCenterLowPass;
    BiquadCascade mSurrLeftMainHP;
    BiquadCascade mSurrLeftCrossHP;
    BiquadCascade mSurrRightMainHP;
    BiquadCascade mSurrRightCrossHP;
    BiquadCascade mSubLowPass;

    // Phase processing
    AllPassChain mPhaseLeftMain;
    AllPassChain mPhaseLeftCross;
    AllPassChain mPhaseRightMain;
    AllPassChain mPhaseRightCross;

    // Timing
    CircularDelay mDelaySurrLeft;
    CircularDelay mDelaySurrRight;

    // Output buffer
    std::vector<float> mSurroundBuffer;
};
