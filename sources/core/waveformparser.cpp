//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
// YouTube channel: https://www.youtube.com/channel/UCz_ktTqWVekT0P4zVW8Xgcg
// YouTube channel e-mail: computerenthusiasttips@mail.ru
//
// Code modification and distribution of any kind is not allowed without direct
// permission of the Author.
//*******************************************************************************

#include "waveformparser.h"
#include "sources/core/experimentaladaptiveparser.h"
#include "sources/models/parsersettingsmodel.h"
#include "sources/models/suspiciouspointsmodel.h"
#include <QPointer>
#include <QDebug>
#include <QDateTime>
#include <QByteArray>
#include <QVariantMap>
#include <QQmlEngine>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <deque>
#include <future>
#include <limits>
#include <numeric>

#define HARDCODED_DATA_SIGNAL_DELTA 0.75

WaveformParser::WaveformParser(QObject* parent) :
    QObject(parent),
    mWavReader(*WavReader::instance()),
    m_experimentalDebugChannel(0),
    m_experimentalDebugActive(false),
    m_experimentalDebugManualInspection(false),
    m_experimentalDebugSample(0),
    m_parsingActive(false),
    m_parsingCancellationRequested(false),
    m_parsingProgress(0)
{

}

// --- Скользящее среднее ---
std::vector<double> movingAverage(const std::vector<double>& signal, size_t window_size) {
    std::vector<double> result(signal.size());
    double sum = 0;
    for (size_t i = 0; i < signal.size(); ++i) {
        sum += signal[i];
        if (i >= window_size) {
            sum -= signal[i - window_size];
        }
        result[i] = (i >= window_size - 1) ? sum / window_size : signal[i];
    }
    return result;
}

// --- Медианный фильтр ---
std::vector<double> medianFilter(const std::vector<double>& signal, size_t window_size) {
    std::vector<double> result(signal.size());
    std::deque<double> window;

    for (size_t i = 0; i < signal.size(); ++i) {
        window.push_back(signal[i]);
        if (window.size() > window_size) {
            window.pop_front();
        }

        std::vector<double> temp(window.begin(), window.end());
        std::sort(temp.begin(), temp.end());
        result[i] = temp[temp.size() / 2];
    }
    return result;
}

// --- Гауссов фильтр ---
std::vector<double> gaussianSmooth(const std::vector<double>& signal, int radius, double sigma) {
    std::vector<double> result(signal.size());
    std::vector<double> kernel(2 * radius + 1);
    double sum = 0;

    // Создание Гауссова ядра
    for (int i = -radius; i <= radius; ++i) {
        kernel[i + radius] = exp(-(i * i) / (2 * sigma * sigma));
        sum += kernel[i + radius];
    }

    // Нормализация
    for (double& val : kernel)
        val /= sum;

    // Применение свёртки
    for (size_t i = 0; i < signal.size(); ++i) {
        double acc = 0;
        for (int j = -radius; j <= radius; ++j) {
            int idx = std::clamp(static_cast<int>(i) + j, 0, static_cast<int>(signal.size() - 1));
            acc += signal[idx] * kernel[j + radius];
        }
        result[i] = acc;
    }

    return result;
}

class AdaptiveKalmanFilter {
public:
    AdaptiveKalmanFilter(double initial_estimate, double window_variance)
        : x_est(last_prediction = initial_estimate), P(1.0), Q(window_variance * 0.01), R(window_variance) {}

    double filter(double measurement) {
        // Prediction step
        double x_pred = x_est;
        double P_pred = P + Q;

        // Kalman gain
        double K = P_pred / (P_pred + R);

        // Update step
        x_est = x_pred + K * (measurement - x_pred);
        P = (1 - K) * P_pred;

        // Адаптация шума измерения и процесса
        double innovation = fabs(measurement - x_pred);
        R = 0.9 * R + 0.1 * (innovation * innovation);  // экспоненциальное сглаживание
        Q = 0.9 * Q + 0.1 * fabs(x_est - last_prediction); // адаптация Q по тренду

        last_prediction = x_est;
        return x_est;
    }

private:
    double x_est;
    double P;
    double Q; // дисперсия модели
    double R; // дисперсия измерений
    double last_prediction;
};

// Подсчёт дисперсии для начального сегмента
double variance(const std::vector<double>& data) {
    double mean = std::accumulate(data.begin(), data.end(), 0.0) / data.size();
    double var = 0.0;
    for (double x: data) {
        var += (x - mean) * (x - mean);
    }
    return var / data.size();
}


/*
int test_kalman() {
    //std::vector<double> signal = {  сюда вставь свой сигнал с шумом  };

    // Оцениваем уровень шума по первым 100 точкам
    std::vector<double> initial_segment(signal.begin(), signal.begin() + 100);
    double initial_variance = variance(initial_segment);

    AdaptiveKalmanFilter kalman(signal[0], initial_variance);

    std::vector<double> filtered;
    for (double s : signal)
        filtered.push_back(kalman.filter(s));

    // Вывод первых значений
    for (size_t i = 0; i < 20; ++i)
        std::cout << "raw = " << signal[i] << ", filtered = " << filtered[i] << std::endl;
}
*/

#include <complex>
const double PI = acos(-1);

typedef std::complex<double> Complex;
typedef std::vector<Complex> CArray;

std::vector<double> autocorrelation(const std::vector<double>& signal) {
    int N = signal.size();
    std::vector<double> result(N, 0.0);

    for (int lag = 0; lag < N; ++lag) {
        double sum = 0.0;
        for (int i = 0; i < N - lag; ++i) {
            sum += signal[i] * signal[i + lag];
        }
        result[lag] = sum;
    }
    return result;
}

// =======================
// FFT (рекурсивная реализация Cooley-Tukey без внешних библиотек)
// =======================
void fft(CArray& x) {
    const size_t N = x.size();
    if (N <= 1) return;

    CArray even(N / 2);
    CArray odd(N / 2);
    for (size_t i = 0; i < N / 2; ++i) {
        even[i] = x[i*2];
        odd[i] = x[i*2 + 1];
    }

    fft(even);
    fft(odd);

    for (size_t k = 0; k < N / 2; ++k) {
        Complex t = std::polar(1.0, -2 * PI * k / N) * odd[k];
        x[k] = even[k] + t;
        x[k + N/2] = even[k] - t;
    }
}

void ifft(CArray& x) {
    const size_t N = x.size();
    if (N <= 1) return;

    CArray even(N / 2);
    CArray odd(N / 2);
    for (size_t i = 0; i < N / 2; ++i) {
        even[i] = x[i * 2];
        odd[i] = x[i * 2 + 1];
    }

    ifft(even);
    ifft(odd);

    for (size_t k = 0; k < N / 2; ++k) {
        Complex t = std::polar(1.0, +2 * PI * k / N) * odd[k];  // знак + !
        x[k] = even[k] + t;
        x[k + N / 2] = even[k] - t;
    }

    // Нормализация после полного преобразования (один раз)
    if (x.size() == N) {
        for (auto& val : x) {
            val /= static_cast<double>(N);
        }
    }
}

void fft2(std::vector<std::complex<double>>& a) {
    int n = a.size();
    if (n <= 1) return;

    std::vector<std::complex<double>> even(n / 2), odd(n / 2);
    for (int i = 0; i < n / 2; ++i) {
        even[i] = a[i*2];
        odd[i] = a[i*2 + 1];
    }

    fft(even);
    fft(odd);

    for (int k = 0; k < n / 2; ++k) {
        std::complex<double> t = std::polar(1.0, -2 * PI * k / n) * odd[k];
        a[k] = even[k] + t;
        a[k + n/2] = even[k] - t;
    }
}

// Обратное БПФ
void ifft2(std::vector<std::complex<double>>& a) {
    for (auto& x : a) x = std::conj(x); // комплексное сопряжение
    fft2(a);
    for (auto& x : a) x = std::conj(x) / static_cast<double>(a.size()); // нормализация
}


void fft_filter_ifft(std::vector<double>& input) {
    int N = input.size();

    // Преобразуем в комплексную форму
    std::vector<std::complex<double>> data(N);
    for (int i = 0; i < N; ++i) {
        data[i] = std::complex<double>(input[i], 0.0);
    }

    // Прямое FFT
    fft2(data);

    // Фильтрация: обнуляем все частоты выше порога
    // int cutoff = N / 16;
    // for (int i = cutoff; i < N - cutoff; ++i) {
    //     data[i] = 0;
    // }

    // Обратное FFT
    ifft2(data);

    // Сохраняем результат
    for (int i = 0; i < N; ++i) {
        input[i] = data[i].real(); // только действительная часть
    }
}

// ======= Метод 2: Скользящее среднее (локальное) =======
void removeDCLocal(std::vector<double>& signal, int windowSize, std::vector<double>& output) {
    int N = signal.size();
    output.resize(N);

    for (int i = 0; i < N; ++i) {
        double sum = 0.0;
        int count = 0;

        for (int j = -windowSize / 2; j <= windowSize / 2; ++j) {
            int idx = i + j;
            if (idx >= 0 && idx < N) {
                sum += signal[idx];
                ++count;
            }
        }

        double local_mean = sum / count;
        output[i] = signal[i] - local_mean;
    }
}

// ======= Метод 3: High-pass фильтр первого порядка =======
void highPassFilter(std::vector<double>& signal, double alpha, std::vector<double>& output) {
    int N = signal.size();
    output.resize(N);
    double y_prev = 0.0, x_prev = 0.0;

    for (int i = 0; i < N; ++i) {
        double x = signal[i];
        double y = alpha * (y_prev + x - x_prev);
        output[i] = y;

        y_prev = y;
        x_prev = x;
    }
}

// Вычисление локальной дисперсии (среднеквадратичное отклонение)
double local_stddev(const std::vector<double>& signal, int center, int half_window) {
    int start = std::max(0, center - half_window);
    int end = std::min((int)signal.size(), center + half_window);
    int size = end - start;
    if (size < 2) return 0.0;

    double mean = 0.0;
    for (int i = start; i < end; ++i) {
        mean += signal[i];
    }
    mean /= size;

    double variance = 0.0;
    for (int i = start; i < end; ++i) {
        variance += (signal[i] - mean) * (signal[i] - mean);
    }
    variance /= (size - 1);
    return std::sqrt(variance);
}

// Адаптивное сглаживание
std::vector<double> adaptive_moving_average(const std::vector<double>& signal) {
    int N = signal.size();
    std::vector<double> result(N);

    for (int i = 0; i < N; ++i) {
        // Вычисляем локальную дисперсию с окном 50 сэмплов
        double stddev = local_stddev(signal, i, 25);

        // Адаптивный выбор окна
        int window = 5;
        // if (stddev < 0.01)
        //     window = 100; // сигнал стабильный
        // else if (stddev < 0.1)
        //     window = 50;
        // else
        //     window = 20; // шумно — берём малое окно

        int start = std::max(0, i - window);
        int end = std::min(N, i + window);
        int size = end - start;

        double sum = 0.0;
        for (int j = start; j < end; ++j) {
            sum += signal[j];
        }
        result[i] = sum / size;
    }

    return result;
}

template <typename T>
T normalizeSignal(std::vector<T>& signal) {
    auto max = std::abs(signal.front());
    for (auto& sample: signal) {
        if (auto as = std::abs(sample); as > max) {
            max = as;
        }
    }
    max *= 1.05;

    for(auto& sample: signal) {
        sample /= max;
    }
    return max;
}

template <typename T>
void denormalizeSignal(std::vector<T>& signal, T max) {
    max /= 1.05;
    for (auto& sample: signal) {
        sample *= max;
    }
}

#include <vector>
#include <cmath>
#include <fftw3.h>

void fft_filter_ifft2(std::vector<double>& input) {
    int N = input.size();

    // Аллоцируем FFTW массивы
    fftw_complex* freq = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    double* time_out = (double*) fftw_malloc(sizeof(double) * N);

    // Прямое FFT
    fftw_plan plan_forward = fftw_plan_dft_r2c_1d(N, input.data(), freq, FFTW_ESTIMATE);
    fftw_execute(plan_forward);

    // Пример фильтрации: обнуляем все частоты выше порога
    int cutoff = N / 16; // оставить только низкие частоты
    for (int i = cutoff; i < N - cutoff; ++i) {
        freq[i][0] = 0.0;
        freq[i][1] = 0.0;
    }

    int low_cutoff = 4; // обрезаем первые 4 низкие частоты (вкл. DC)
    for (int i = 0; i < low_cutoff; ++i) {
        freq[i][0] = 0.0;
        freq[i][1] = 0.0;
    }

    // Обратное FFT
    fftw_plan plan_inverse = fftw_plan_dft_c2r_1d(N, freq, time_out, FFTW_ESTIMATE);
    fftw_execute(plan_inverse);

    // Нормализация и сохранение результата
    for (int i = 0; i < N; ++i) {
        input[i] = time_out[i] / N;
    }

    // Очистка
    fftw_destroy_plan(plan_forward);
    fftw_destroy_plan(plan_inverse);
    fftw_free(freq);
    fftw_free(time_out);
}


struct FilterConfig {
    bool remove_dc = true;
    bool apply_smooth = true;
    bool use_median = false;
    bool use_gaussian = false;
    bool use_fft_filter = true;
    bool use_kalman = true;
    int smooth_window = 5;
    int fft_cutoff = -1;
    double process_noise = 1e-5;
    double measurement_noise = 1e-2;
};

double calc_stddev(const std::vector<double>& v) {
    double mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
    double accum = 0;
    for (auto x : v) accum += (x - mean) * (x - mean);
    return std::sqrt(accum / v.size());
}

void auto_tune_config(const std::vector<double>& signal, FilterConfig& cfg) {
    double stddev = calc_stddev(signal);

    // Автоопределение окна сглаживания
    if (stddev > 0.05) {
        cfg.smooth_window = std::min(15, std::max(3, int(stddev * 100)));
        cfg.use_gaussian = true;
    }

    // Проверка на выбросы -> медианный фильтр
    int spikes = 0;
    for (size_t i = 1; i < signal.size(); ++i) {
        if (std::abs(signal[i] - signal[i - 1]) > stddev * 4) spikes++;
    }
    if (spikes > signal.size() * 0.01) {
        cfg.use_median = true;
        cfg.use_gaussian = false;
    }

    // Оценка спектра для FFT-фильтра
    int N = signal.size();
    fftw_complex* freq = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_plan plan = fftw_plan_dft_r2c_1d(N, const_cast<double*>(signal.data()), freq, FFTW_ESTIMATE);
    fftw_execute(plan);

    double max_amp = 0;
    int cutoff = N / 2;
    for (int i = 1; i < N / 2; ++i) {
        double mag = sqrt(freq[i][0] * freq[i][0] + freq[i][1] * freq[i][1]);
        if (mag > max_amp) {
            max_amp = mag;
            cutoff = i;
        }
    }
    cfg.fft_cutoff = std::max(8, cutoff / 4);  // оставим часть с пиком

    fftw_destroy_plan(plan);
    fftw_free(freq);

    // Автонастройка Калмана
    cfg.process_noise = stddev * 1e-3;
    cfg.measurement_noise = stddev * 1e-1;
}

// DC-смещение
void remove_dc_offset(std::vector<double>& input) {
    double mean = std::accumulate(input.begin(), input.end(), 0.0) / input.size();
    for (auto& x : input) x -= mean;
}

// Медианное сглаживание
void median_filter(std::vector<double>& data, int window) {
    std::vector<double> result(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        int start = std::max<int>(0, i - window);
        int end = std::min<int>(data.size() - 1, i + window);
        std::vector<double> win(data.begin() + start, data.begin() + end + 1);
        std::nth_element(win.begin(), win.begin() + win.size() / 2, win.end());
        result[i] = win[win.size() / 2];
    }
    data = result;
}

// Гауссово сглаживание
void gaussian_smooth(std::vector<double>& data, int window_size) {
    std::vector<double> kernel(2 * window_size + 1);
    double sigma = window_size / 2.0;
    double sum = 0;
    for (int i = -window_size; i <= window_size; ++i) {
        kernel[i + window_size] = std::exp(-0.5 * (i * i) / (sigma * sigma));
        sum += kernel[i + window_size];
    }
    for (auto& x : kernel) x /= sum;

    std::vector<double> result(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        double acc = 0, norm = 0;
        for (int j = -window_size; j <= window_size; ++j) {
            int idx = i + j;
            if (idx >= 0 && idx < data.size()) {
                acc += data[idx] * kernel[j + window_size];
                norm += kernel[j + window_size];
            }
        }
        result[i] = acc / norm;
    }
    data = result;
}

// Простое скользящее среднее
void smooth_moving_average(std::vector<double>& data, int window) {
    std::vector<double> result(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        double sum = 0;
        int count = 0;
        for (int j = std::max<int>(0, i - window); j <= std::min<int>(data.size() - 1, i + window); ++j) {
            sum += data[j];
            ++count;
        }
        result[i] = sum / count;
    }
    data = result;
}

// FFT-фильтрация
void fft_filter(std::vector<double>& input) {
    int N = input.size();
    fftw_complex* freq = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    double* time_out = (double*) fftw_malloc(sizeof(double) * N);

    fftw_plan plan_forward = fftw_plan_dft_r2c_1d(N, input.data(), freq, FFTW_ESTIMATE);
    fftw_execute(plan_forward);

    int cutoff = N / 16;
    for (int i = cutoff; i < N - cutoff; ++i) {
        freq[i][0] = 0;
        freq[i][1] = 0;
    }

    fftw_plan plan_inverse = fftw_plan_dft_c2r_1d(N, freq, time_out, FFTW_ESTIMATE);
    fftw_execute(plan_inverse);

    for (int i = 0; i < N; ++i)
        input[i] = time_out[i] / N;

    fftw_destroy_plan(plan_forward);
    fftw_destroy_plan(plan_inverse);
    fftw_free(freq);
    fftw_free(time_out);
}

// Калман
void kalman_filter(std::vector<double>& input, double q, double r) {
    double estimate = 0.0;
    double error_estimate = 1.0;

    for (auto& x : input) {
        double kalman_gain = error_estimate / (error_estimate + r);
        estimate = estimate + kalman_gain * (x - estimate);
        error_estimate = (1.0 - kalman_gain) * error_estimate + fabs(estimate) * q;
        x = estimate;
    }
}

// Главная функция обработки
void process_signal(std::vector<double>& signal, const FilterConfig& cfg) {
    if (cfg.remove_dc) remove_dc_offset(signal);
    if (cfg.apply_smooth) {
        if (cfg.use_median)
            median_filter(signal, cfg.smooth_window);
        else if (cfg.use_gaussian)
            gaussian_smooth(signal, cfg.smooth_window);
        else
            smooth_moving_average(signal, cfg.smooth_window);
    }
    if (cfg.use_fft_filter) fft_filter(signal);
    if (cfg.use_kalman) kalman_filter(signal, cfg.process_noise, cfg.measurement_noise);
}

// Step 1: Remove DC offset
void remove_dc_offset2(std::vector<double>& signal) {
    double mean = std::accumulate(signal.begin(), signal.end(), 0.0) / signal.size();
    for (auto& sample : signal) {
        sample -= mean;
    }
}

// Step 2: Normalize amplitude to [-1, 1]
void normalize_amplitude(std::vector<double>& signal) {
    double max_val = *std::max_element(signal.begin(), signal.end(),
                                       [](double a, double b) { return std::abs(a) < std::abs(b); });
    if (max_val == 0.0) return;
    for (auto& sample : signal) {
        sample /= max_val;
    }
}

// Step 3: Band-pass filter using FFT (1200–3600 Hz)
void bandpass_filter(std::vector<double>& signal, int sample_rate) {
    int N = signal.size();
    fftw_complex* freq = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N/2 + 1));
    double* time_out = (double*) fftw_malloc(sizeof(double) * N);

    fftw_plan plan_forward = fftw_plan_dft_r2c_1d(N, signal.data(), freq, FFTW_ESTIMATE);
    fftw_plan plan_inverse = fftw_plan_dft_c2r_1d(N, freq, time_out, FFTW_ESTIMATE);

    fftw_execute(plan_forward);

    double freq_resolution = static_cast<double>(sample_rate) / N;
    int low_bin = static_cast<int>(1200 / freq_resolution);
    int high_bin = static_cast<int>(3600 / freq_resolution);

    for (int i = 0; i <= N/2; ++i) {
        if (i < low_bin || i > high_bin) {
            freq[i][0] = 0.0;
            freq[i][1] = 0.0;
        }
    }

    fftw_execute(plan_inverse);

    // Normalize inverse FFT result
    for (int i = 0; i < N; ++i) {
        signal[i] = time_out[i] / N;
    }

    fftw_destroy_plan(plan_forward);
    fftw_destroy_plan(plan_inverse);
    fftw_free(freq);
    fftw_free(time_out);
}

void fft_soft_bandpass(std::vector<double>& signal, double sampleRate) {
    int N = signal.size();
    fftw_complex* freq = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (N / 2 + 1));
    double* output = (double*)fftw_malloc(sizeof(double) * N);

    fftw_plan forward = fftw_plan_dft_r2c_1d(N, signal.data(), freq, FFTW_ESTIMATE);
    fftw_plan inverse = fftw_plan_dft_c2r_1d(N, freq, output, FFTW_ESTIMATE);

    fftw_execute(forward);

    // Центр пропускания
    double centerFreq = 2400.0;
    double bandwidth = 1200.0;

    for (int k = 0; k <= N / 2; ++k) {
        double freq_hz = k * sampleRate / N;
        double distance = (freq_hz - centerFreq) / (bandwidth / 2.0);
        double weight = exp(-distance * distance); // Гауссово окно

        freq[k][0] *= weight;
        freq[k][1] *= weight;
    }

    fftw_execute(inverse);

    // Нормализация
    for (int i = 0; i < N; ++i)
        signal[i] = output[i] / N;

    fftw_destroy_plan(forward);
    fftw_destroy_plan(inverse);
    fftw_free(freq);
    fftw_free(output);
}

#include "wavelib.h"  // библиотека вейвлетов

void waveletRemoveLocalDrift(std::vector<double>& signal, int windowSize = 256, double driftThreshold = 0.1f) {
    int N = signal.size();
    std::vector<double> result(N, 0.0f);
    std::vector<int> count(N, 0);

    // Создаём вейвлет (например, Daubechies 4)
    wave_object wave = wave_init("db4");
    int levels = 6;

    // WT требует длину, кратную степени двойки
    int wt_len = 1;
    while (wt_len < windowSize) wt_len <<= 1;

    wt_object wt = wt_init(wave, "dwt", wt_len, levels);
    setDWTExtension(wt, "sym");  // симметричное расширение

    for (int start = 0; start < N; start += windowSize / 2) {
        int end = std::min(start + windowSize, N);
        int len = end - start;

        std::vector<double> windowData(wt_len, 0.0f);
        for (int i = 0; i < len; ++i) {
            windowData[i] = signal[start + i];
        }

        // Прямое DWT
        dwt(wt, windowData.data());

        double* approx = wt->output;
        int alen = wt->length[0];

        // Анализ дрейфа в аппроксимации
        double maxDrift = 0.0f;
        for (int i = 1; i < alen; ++i) {
            maxDrift = std::max(maxDrift, std::fabs(approx[i] - approx[i - 1]));
        }

        if (maxDrift > driftThreshold) {
            for (int i = 0; i < alen; ++i) {
                approx[i] *= 0.05f;  // подавляем дрейф
            }
        }

        // Обратное DWT
        idwt(wt, wt->output);

        // Усреднение перекрытий
        for (int i = 0; i < len; ++i) {
            int idx = start + i;
            result[idx] += wt->output[i];
            count[idx]++;
        }
    }

    for (int i = 0; i < N; ++i) {
        if (count[i] > 0) signal[i] = result[i] / count[i];
    }

    wt_free(wt);
    wave_free(wave);
}

void WaveformParser::repairWaveform3(uint chNum) {
    if (chNum >= mWavReader.getNumberOfChannels()) {
        qDebug() << "Trying to parse channel that exceeds overall number of channels";
        return;
    }

    QWavVector& channel = *(chNum == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1());
    auto raw = std::vector<double>(channel.begin(), channel.end());
    //std::vector<double> res(raw.size(), 0.0);
    auto max = normalizeSignal(raw);

    //int sample_rate = 48000; // Example sample rate, use real value from WAV file


    waveletRemoveLocalDrift(raw, 386, 0.1);

    // remove_dc_offset2(raw);
    // normalize_amplitude(raw);
    // bandpass_filter(raw, sample_rate);

//    fft_soft_bandpass(raw, sample_rate);

    //highPassFilter(raw,  0.995, res);
    //fft_filter_ifft2(raw);
    auto res = raw;

    denormalizeSignal(res, max);

    auto pos = channel.size() - channel.size();
//    auto res = autocorrelation(raw);
    for (auto i: res) {
        channel[pos++] = i;
    }
    /*

    // 1. Предобработка
    auto smoothed1 = medianFilter(raw, 5);           // устойчивость к выбросам
    auto smoothed2 = gaussianSmooth(smoothed1, 4, 1); // мягкое сглаживание
    auto smoothed3 = movingAverage(smoothed2, 5);     // сглаживание колебаний

    // 2. Оценка начального шума
    auto initial_segment = std::vector<double>(smoothed3.begin(), smoothed3.begin() + 100);
    double init_var = variance(initial_segment);

    // 3. Адаптивный фильтр Калмана
    AdaptiveKalmanFilter kalman(smoothed3[0], init_var);
    std::vector<double> final;

    auto pos = raw.size() - raw.size();
    for (auto& i: channel) {
        i = kalman.filter(smoothed3[pos++]);
    }

    // for (double val: smoothed3) {
    //     final.push_back(kalman.filter(val));
    // }
    */
}
/*
void WaveformParser::repairWaveform2(uint chNum) {
    if (chNum >= mWavReader.getNumberOfChannels()) {
        qDebug() << "Trying to parse channel that exceeds overall number of channels";
        return;
    }

    QWavVector& channel = *(chNum == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1());
    setParsingProgress(true, 0, QString("Channel %1: scanning waveform").arg(chNum + 1));
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    QVector<ParsedData::WaveformPart> parsed = parseChannel<QWavVectorType>(channel);

    const auto& parserSettings = ParserSettingsModel::instance()->getParserSettings();
    const double sampleRate = mWavReader.getSampleRate();
    for (auto it { parsed.begin() }; it != parsed.end();) {
        auto itprev = it++;
        if (it != parsed.end()) {
            bool isZero = isZeroFreqFitsInDelta(sampleRate, (*it).length + (*itprev).length, parserSettings.zeroFreq, parserSettings.zeroDelta, HARDCODED_DATA_SIGNAL_DELTA);
            bool isOne = isOneFreqFitsInDelta(sampleRate, (*it).length + (*itprev).length, parserSettings.oneFreq, HARDCODED_DATA_SIGNAL_DELTA, parserSettings.oneDelta);
            if (isZero || isOne) {
                auto it1 = std::next(channel.begin(), (*itprev).begin);
                auto it2 = std::next(channel.begin(), (*it).end);
                auto itmiddle = std::next(it1, std::distance(it1, it2) / 2);
                const auto min_max = std::minmax_element(it1, it2);
                auto [val1, val2] = (*itprev).sign == ParsedData::WaveformSign::NEGATIVE ? min_max : decltype(min_max) {min_max.second, min_max.first};
                auto itr = it1;
                for (; itr != itmiddle; ++itr) {
                    *itr  = *val1;
                }
                for (; itr != it2; ++itr) {
                    *itr = *val2;
                }
            }
            ++it;
        }
    }
}
*/

inline ParsedData* WaveformParser::getOrCreateParsedDataPtr(uint chNum)
{
    auto it = m_parsedData.find(chNum);
    return it == m_parsedData.end() ? *m_parsedData.insert(chNum, new ParsedData(this)) : *it;
}

inline ParsedData* WaveformParser::getParsedDataPtr(uint chNum) const
{
    auto parsedDataIt { m_parsedData.find(chNum) };
    if (parsedDataIt == m_parsedData.end()) {
        qWarning() << "Unable to access parsed data with requested channel number.";
        return nullptr;
    }
    return *parsedDataIt;
}

QSharedPointer<QVector<QSharedPointer<ParsedData::DataBlock>>> WaveformParser::getParsedDataSharedPtr(uint chNum) const
{
    auto p = getParsedDataPtr(chNum);
    return p == nullptr ? QSharedPointer<QVector<QSharedPointer<ParsedData::DataBlock>>> { } : p->getParsedData();
}

inline bool WaveformParser::isZeroFreqFitsInDelta(uint32_t sampleRate, uint32_t length, uint32_t signalFreq, double signalDeltaBelow, double signalDeltaAbove) const
{
    return isFreqFitsInDelta2(sampleRate, length, signalFreq, signalDeltaBelow, signalDeltaAbove);
}

inline bool WaveformParser::isOneFreqFitsInDelta(uint32_t sampleRate, uint32_t length, uint32_t signalFreq, double signalDeltaBelow, double signalDeltaAbove) const
{
    return isFreqFitsInDelta2(sampleRate, length, signalFreq, signalDeltaBelow, signalDeltaAbove);
}

void WaveformParser::setParsingProgress(bool active, int progress, const QString& status)
{
    progress = std::clamp(progress, 0, 100);
    if (m_parsingActive == active && m_parsingProgress == progress && m_parsingStatus == status) {
        return;
    }

    m_parsingActive = active;
    m_parsingProgress = progress;
    m_parsingStatus = status;
    emit parsingProgressChanged();
}

double WaveformParser::scoreExperimentalWindow(const QWavVector& channel, size_t begin, size_t length, double* axis, double* upperLevel, double* lowerLevel) const
{
    if (length < 8 || begin >= static_cast<size_t>(channel.size()) || begin + length > static_cast<size_t>(channel.size())) {
        return 0.0;
    }

    std::vector<double> samples;
    samples.reserve(length);
    for (size_t pos { begin }; pos < begin + length; ++pos) {
        samples.push_back(channel.at(static_cast<qsizetype>(pos)));
    }

    auto sortedSamples { samples };
    const auto medianIt { std::next(sortedSamples.begin(), static_cast<std::ptrdiff_t>(sortedSamples.size() / 2)) };
    std::nth_element(sortedSamples.begin(), medianIt, sortedSamples.end());
    const double virtualAxis { *medianIt };
    if (axis) {
        *axis = virtualAxis;
    }

    const auto [minIt, maxIt] { std::minmax_element(samples.cbegin(), samples.cend()) };
    const double range { *maxIt - *minIt };
    if (range <= 0.01) {
        return 0.0;
    }

    int bucketCount { std::clamp(static_cast<int>(length / 6), 8, 24) };
    bucketCount = std::min(bucketCount, static_cast<int>(length / 2));
    if (bucketCount < 4) {
        return 0.0;
    }

    QVector<int> bucketSigns;
    QVector<double> bucketWeights;
    QVector<double> positiveBuckets;
    QVector<double> negativeBuckets;
    bucketSigns.reserve(bucketCount);
    bucketWeights.reserve(bucketCount);
    const double weakBucketThreshold { range * 0.05 };
    for (int bucket { 0 }; bucket < bucketCount; ++bucket) {
        const size_t bucketBegin { length * static_cast<size_t>(bucket) / static_cast<size_t>(bucketCount) };
        const size_t bucketEnd { length * static_cast<size_t>(bucket + 1) / static_cast<size_t>(bucketCount) };
        double sum { 0.0 };
        for (size_t sample { bucketBegin }; sample < bucketEnd; ++sample) {
            sum += samples.at(sample) - virtualAxis;
        }

        const double mean { sum / std::max<size_t>(1, bucketEnd - bucketBegin) };
        bucketSigns.append(std::fabs(mean) < weakBucketThreshold ? 0 : (mean > 0.0 ? 1 : -1));
        bucketWeights.append(std::max(std::fabs(mean), weakBucketThreshold));
        if (mean > weakBucketThreshold) {
            positiveBuckets.append(virtualAxis + mean);
        } else if (mean < -weakBucketThreshold) {
            negativeBuckets.append(virtualAxis + mean);
        }
    }

    const auto medianValue = [](QVector<double> values, double fallback) {
        if (values.empty()) {
            return fallback;
        }

        const auto median { std::next(values.begin(), values.size() / 2) };
        std::nth_element(values.begin(), median, values.end());
        return *median;
    };
    if (upperLevel) {
        *upperLevel = medianValue(positiveBuckets, *maxIt);
    }
    if (lowerLevel) {
        *lowerLevel = medianValue(negativeBuckets, *minIt);
    }

    auto scorePolarity = [&bucketSigns, &bucketWeights, bucketCount](int split, int firstSign) {
        double matchedWeight { 0.0 };
        double totalWeight { 0.0 };
        int firstStrongBuckets { 0 };
        int secondStrongBuckets { 0 };
        for (int bucket { 0 }; bucket < bucketCount; ++bucket) {
            const int expectedSign { bucket < split ? firstSign : -firstSign };
            const int sign { bucketSigns.at(bucket) };
            const double weight { bucketWeights.at(bucket) };
            totalWeight += weight;
            if (sign == expectedSign) {
                matchedWeight += weight;
                if (bucket < split) {
                    ++firstStrongBuckets;
                } else {
                    ++secondStrongBuckets;
                }
            } else if (sign == 0) {
                matchedWeight += weight * 0.5;
            }
        }

        if (firstStrongBuckets < split / 2 || secondStrongBuckets < (bucketCount - split) / 2) {
            return 0.0;
        }

        int signChanges { 0 };
        int previousSign { 0 };
        for (const int sign: bucketSigns) {
            if (sign == 0) {
                continue;
            }

            if (previousSign != 0 && sign != previousSign) {
                ++signChanges;
            }
            previousSign = sign;
        }

        const double roughnessPenalty { std::max(0, signChanges - 1) * 0.035 };
        return totalWeight > 0.0 ? std::max(0.0, matchedWeight / totalWeight - roughnessPenalty) : 0.0;
    };

    double bestScore { 0.0 };
    const int minSplit { std::max(2, static_cast<int>(std::floor(bucketCount * 0.35))) };
    const int maxSplit { std::min(bucketCount - 2, static_cast<int>(std::ceil(bucketCount * 0.65))) };
    for (int split { minSplit }; split <= maxSplit; ++split) {
        bestScore = std::max(bestScore, scorePolarity(split, 1));
        bestScore = std::max(bestScore, scorePolarity(split, -1));
    }

    double bestPlateauScore { 0.0 };
    const auto scorePlateauPolarity = [&samples, virtualAxis, range](size_t split, int firstSign) {
        double firstSum { 0.0 };
        double secondSum { 0.0 };
        int firstMatchingSamples { 0 };
        int secondMatchingSamples { 0 };
        const size_t secondSize { samples.size() - split };
        for (size_t sample { 0 }; sample < split; ++sample) {
            const double value { samples.at(sample) - virtualAxis };
            firstSum += value;
            if ((firstSign > 0 && value > 0.0) || (firstSign < 0 && value < 0.0)) {
                ++firstMatchingSamples;
            }
        }
        for (size_t sample { split }; sample < samples.size(); ++sample) {
            const double value { samples.at(sample) - virtualAxis };
            secondSum += value;
            if ((firstSign > 0 && value < 0.0) || (firstSign < 0 && value > 0.0)) {
                ++secondMatchingSamples;
            }
        }

        const double firstMean { firstSum / std::max<size_t>(1, split) };
        const double secondMean { secondSum / std::max<size_t>(1, secondSize) };
        if ((firstSign > 0 && (firstMean <= 0.0 || secondMean >= 0.0)) ||
                (firstSign < 0 && (firstMean >= 0.0 || secondMean <= 0.0))) {
            return 0.0;
        }

        const double firstDominance { firstMatchingSamples / static_cast<double>(std::max<size_t>(1, split)) };
        const double secondDominance { secondMatchingSamples / static_cast<double>(std::max<size_t>(1, secondSize)) };
        const double dominanceScore { std::min(firstDominance, secondDominance) };
        const double strengthScore { std::clamp(std::min(std::fabs(firstMean), std::fabs(secondMean)) / (range * 0.18), 0.0, 1.0) };
        const double balanceScore { 1.0 - std::clamp(std::fabs(static_cast<double>(split) / samples.size() - 0.5) / 0.2, 0.0, 1.0) };
        return dominanceScore * 0.55 + strengthScore * 0.30 + balanceScore * 0.15;
    };

    const size_t minPlateauSplit { std::max<size_t>(2, static_cast<size_t>(std::floor(samples.size() * 0.35))) };
    const size_t maxPlateauSplit { std::min(samples.size() - 2, static_cast<size_t>(std::ceil(samples.size() * 0.65))) };
    for (size_t split { minPlateauSplit }; split <= maxPlateauSplit; ++split) {
        bestPlateauScore = std::max(bestPlateauScore, scorePlateauPolarity(split, 1));
        bestPlateauScore = std::max(bestPlateauScore, scorePlateauPolarity(split, -1));
    }

    const bool normalizedFloat { std::max(std::fabs(*minIt), std::fabs(*maxIt)) <= 2.0 };
    const double amplitudeScale { normalizedFloat ? 0.25 : 2500.0 };
    const double amplitudeScore { std::clamp(range / amplitudeScale, 0.0, 1.0) };
    const double shapeScore { std::max(bestScore, bestPlateauScore) };
    return std::clamp(shapeScore * 0.9 + amplitudeScore * 0.1, 0.0, 1.0);
}

QVariantMap WaveformParser::findExperimentalBitCandidate(const QWavVector& channel, size_t expectedBegin, uint8_t bit, const ParserSettingsModel::ParserSettings& parserSettings, double sampleRate) const
{
    const int signalFreq { bit == 0 ? parserSettings.zeroFreq : parserSettings.oneFreq };
    const double deltaBelow { bit == 0 ? parserSettings.zeroDelta : HARDCODED_DATA_SIGNAL_DELTA };
    const double deltaAbove { bit == 0 ? HARDCODED_DATA_SIGNAL_DELTA : parserSettings.oneDelta };
    const int expectedLength { static_cast<int>(std::lround(sampleRate / signalFreq)) };
    const int minLength { std::max(8, static_cast<int>(std::floor(sampleRate / (signalFreq * (1.0 + deltaAbove))))) };
    const int maxLength { std::max(minLength, static_cast<int>(std::ceil(sampleRate / (signalFreq * (1.0 - std::min(deltaBelow, 0.95)))))) };
    const int startJitter { std::max(2, expectedLength / 8) };

    QVariantMap best {
        { "valid", false },
        { "bit", bit },
        { "begin", static_cast<int>(expectedBegin) },
        { "end", static_cast<int>(expectedBegin) },
        { "expectedLength", expectedLength },
        { "minLength", minLength },
        { "maxLength", maxLength },
        { "startJitter", startJitter },
        { "referenceBegin", static_cast<int>(expectedBegin) },
        { "referenceExpectedEnd", static_cast<int>(expectedBegin + static_cast<size_t>(expectedLength) - 1) },
        { "referenceMinEnd", static_cast<int>(expectedBegin + static_cast<size_t>(minLength) - 1) },
        { "referenceMaxEnd", static_cast<int>(expectedBegin + static_cast<size_t>(maxLength) - 1) },
        { "rawScore", 0.0 },
        { "startPenalty", 0.0 },
        { "lengthPenalty", 0.0 },
        { "greedyOnePenalty", 0.0 },
        { "zeroPrefixScore", 0.0 },
        { "totalPenalty", 0.0 },
        { "score", 0.0 },
        { "axis", 0.0 },
        { "upperLevel", 0.0 },
        { "lowerLevel", 0.0 },
        { "range", 0.0 },
        { "upperSample", static_cast<int>(expectedBegin) },
        { "lowerSample", static_cast<int>(expectedBegin) },
        { "upperPointValue", 0.0 },
        { "lowerPointValue", 0.0 },
    };

    for (int startOffset { -startJitter }; startOffset <= startJitter; ++startOffset) {
        if (startOffset < 0 && expectedBegin < static_cast<size_t>(-startOffset)) {
            continue;
        }
        const size_t begin { static_cast<size_t>(static_cast<qint64>(expectedBegin) + startOffset) };
        for (int length { minLength }; length <= maxLength; ++length) {
            double axis { 0.0 };
            double upperLevel { 0.0 };
            double lowerLevel { 0.0 };
            double score { scoreExperimentalWindow(channel, begin, static_cast<size_t>(length), &axis, &upperLevel, &lowerLevel) };
            if (score <= 0.0) {
                continue;
            }

            const double rawScore { score };
            const double startPenalty { std::fabs(startOffset) / static_cast<double>(std::max(1, startJitter)) * 0.08 };
            const double lengthPenalty { std::fabs(length - expectedLength) / static_cast<double>(std::max(1, expectedLength)) * (bit == 0 ? 0.22 : 0.16) };
            const double totalPenalty { startPenalty + lengthPenalty };
            score = std::max(0.0, score - totalPenalty);
            if (!best["valid"].toBool() || score > best["score"].toDouble()) {
                best["valid"] = true;
                best["begin"] = static_cast<int>(begin);
                best["end"] = static_cast<int>(begin + static_cast<size_t>(length) - 1);
                best["length"] = length;
                best["rawScore"] = rawScore;
                best["startPenalty"] = startPenalty;
                best["lengthPenalty"] = lengthPenalty;
                best["totalPenalty"] = totalPenalty;
                best["score"] = score;
                best["axis"] = axis;
                best["upperLevel"] = upperLevel;
                best["lowerLevel"] = lowerLevel;
            }
        }
    }

    if (best["valid"].toBool()) {
        const int begin { best["begin"].toInt() };
        const int end { best["end"].toInt() };
        int upperSample { begin };
        int lowerSample { begin };
        double upperValue { channel.at(begin) };
        double lowerValue { channel.at(begin) };
        for (int sample { begin + 1 }; sample <= end; ++sample) {
            const double value { channel.at(sample) };
            if (value > upperValue) {
                upperValue = value;
                upperSample = sample;
            }
            if (value < lowerValue) {
                lowerValue = value;
                lowerSample = sample;
            }
        }

        best["upperSample"] = upperSample;
        best["lowerSample"] = lowerSample;
        best["upperPointValue"] = upperValue;
        best["lowerPointValue"] = lowerValue;
        best["range"] = upperValue - lowerValue;
    }

    return best;
}

QVariantMap WaveformParser::findExperimentalPeriodCandidate(const QWavVector& channel, size_t expectedBegin, const ParserSettingsModel::ParserSettings& parserSettings, double sampleRate) const
{
    const int zeroExpectedLength { static_cast<int>(std::lround(sampleRate / parserSettings.zeroFreq)) };
    const int oneExpectedLength { static_cast<int>(std::lround(sampleRate / parserSettings.oneFreq)) };
    const int zeroMinLength { std::max(8, static_cast<int>(std::floor(sampleRate / (parserSettings.zeroFreq * (1.0 + HARDCODED_DATA_SIGNAL_DELTA))))) };
    const int zeroMaxLength { std::max(zeroMinLength, static_cast<int>(std::ceil(sampleRate / (parserSettings.zeroFreq * (1.0 - std::min(parserSettings.zeroDelta, 0.95)))))) };
    const int oneMinLength { std::max(8, static_cast<int>(std::floor(sampleRate / (parserSettings.oneFreq * (1.0 + parserSettings.oneDelta))))) };
    const int oneMaxLength { std::max(oneMinLength, static_cast<int>(std::ceil(sampleRate / (parserSettings.oneFreq * (1.0 - std::min(HARDCODED_DATA_SIGNAL_DELTA, 0.95)))))) };
    const int minLength { std::min(zeroMinLength, oneMinLength) };
    const int maxLength { std::max(zeroMaxLength, oneMaxLength) };
    const int classificationBoundary { (zeroMaxLength + oneMinLength) / 2 };
    const int startJitter { std::max(2, zeroExpectedLength / 8) };

    QVariantMap best {
        { "valid", false },
        { "bit", -1 },
        { "begin", static_cast<int>(expectedBegin) },
        { "end", static_cast<int>(expectedBegin) },
        { "expectedLength", 0 },
        { "zeroExpectedLength", zeroExpectedLength },
        { "oneExpectedLength", oneExpectedLength },
        { "minLength", minLength },
        { "maxLength", maxLength },
        { "startJitter", startJitter },
        { "classificationBoundary", classificationBoundary },
        { "referenceBegin", static_cast<int>(expectedBegin) },
        { "referenceExpectedEnd", static_cast<int>(expectedBegin + static_cast<size_t>(classificationBoundary) - 1) },
        { "referenceMinEnd", static_cast<int>(expectedBegin + static_cast<size_t>(minLength) - 1) },
        { "referenceMaxEnd", static_cast<int>(expectedBegin + static_cast<size_t>(maxLength) - 1) },
        { "rawScore", 0.0 },
        { "startPenalty", 0.0 },
        { "lengthPenalty", 0.0 },
        { "totalPenalty", 0.0 },
        { "score", 0.0 },
        { "axis", 0.0 },
        { "upperLevel", 0.0 },
        { "lowerLevel", 0.0 },
        { "range", 0.0 },
        { "upperSample", static_cast<int>(expectedBegin) },
        { "lowerSample", static_cast<int>(expectedBegin) },
        { "upperPointValue", 0.0 },
        { "lowerPointValue", 0.0 },
        { "detector", QString("period") },
    };

    for (int startOffset { -startJitter }; startOffset <= startJitter; ++startOffset) {
        if (startOffset < 0 && expectedBegin < static_cast<size_t>(-startOffset)) {
            continue;
        }

        const size_t begin { static_cast<size_t>(static_cast<qint64>(expectedBegin) + startOffset) };
        for (int length { minLength }; length <= maxLength; ++length) {
            double axis { 0.0 };
            double upperLevel { 0.0 };
            double lowerLevel { 0.0 };
            double score { scoreExperimentalWindow(channel, begin, static_cast<size_t>(length), &axis, &upperLevel, &lowerLevel) };
            if (score <= 0.0) {
                continue;
            }

            const int bit { length >= classificationBoundary ? 1 : 0 };
            const int expectedLength { bit == 0 ? zeroExpectedLength : oneExpectedLength };
            const double rawScore { score };
            const double startPenalty { std::fabs(startOffset) / static_cast<double>(std::max(1, startJitter)) * 0.06 };
            const double lengthPenalty { std::fabs(length - expectedLength) / static_cast<double>(std::max(1, expectedLength)) * 0.08 };
            double greedyOnePenalty { 0.0 };
            double zeroPrefixScore { 0.0 };
            if (bit == 1 && zeroExpectedLength >= minLength && zeroExpectedLength < length) {
                zeroPrefixScore = scoreExperimentalWindow(channel, begin, static_cast<size_t>(zeroExpectedLength));
                if (zeroPrefixScore >= 0.82 && rawScore - zeroPrefixScore <= 0.14) {
                    greedyOnePenalty = (0.14 - (rawScore - zeroPrefixScore)) * 1.5;
                }
            }
            const double totalPenalty { startPenalty + lengthPenalty + greedyOnePenalty };
            score = std::max(0.0, score - totalPenalty);
            if (!best["valid"].toBool() || score > best["score"].toDouble()) {
                best["valid"] = true;
                best["bit"] = bit;
                best["begin"] = static_cast<int>(begin);
                best["end"] = static_cast<int>(begin + static_cast<size_t>(length) - 1);
                best["length"] = length;
                best["expectedLength"] = expectedLength;
                best["referenceExpectedEnd"] = static_cast<int>(expectedBegin + static_cast<size_t>(expectedLength) - 1);
                best["rawScore"] = rawScore;
                best["startPenalty"] = startPenalty;
                best["lengthPenalty"] = lengthPenalty;
                best["greedyOnePenalty"] = greedyOnePenalty;
                best["zeroPrefixScore"] = zeroPrefixScore;
                best["totalPenalty"] = totalPenalty;
                best["score"] = score;
                best["axis"] = axis;
                best["upperLevel"] = upperLevel;
                best["lowerLevel"] = lowerLevel;
            }
        }
    }

    if (best["valid"].toBool()) {
        const int begin { best["begin"].toInt() };
        const int end { best["end"].toInt() };
        int upperSample { begin };
        int lowerSample { begin };
        double upperValue { channel.at(begin) };
        double lowerValue { channel.at(begin) };
        for (int sample { begin + 1 }; sample <= end; ++sample) {
            const double value { channel.at(sample) };
            if (value > upperValue) {
                upperValue = value;
                upperSample = sample;
            }
            if (value < lowerValue) {
                lowerValue = value;
                lowerSample = sample;
            }
        }

        best["upperSample"] = upperSample;
        best["lowerSample"] = lowerSample;
        best["upperPointValue"] = upperValue;
        best["lowerPointValue"] = lowerValue;
        best["range"] = upperValue - lowerValue;
    }

    return best;
}

void WaveformParser::parse(uint chNum)
{
    if (m_parsingCancellationRequested) {
        return;
    }

    if (chNum >= mWavReader.getNumberOfChannels()) {
        qDebug() << "Trying to parse channel that exceeds overall number of channels";
        return;
    }

    setParsingProgress(true, 0, QString("Channel %1: preparing parser").arg(chNum + 1));
    QCoreApplication::processEvents();

    QWavVector& channel = *(chNum == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1());
    setParsingProgress(true, 0, QString("Channel %1: detecting half-waves").arg(chNum + 1));
    QCoreApplication::processEvents();
    QVector<ParsedData::WaveformPart> parsed = parseChannel<QWavVectorType>(channel);

    const double sampleRate = mWavReader.getSampleRate();
    auto& parsedData = *getOrCreateParsedDataPtr(chNum);
    parsedData.clear(channel.size());
    mSelectedBlocks[chNum].clear();

    auto currentState = SEARCH_OF_PILOT_TONE;
    auto it = parsed.begin();
    m_experimentalDebugManualInspection = false;
    QElapsedTimer progressTimer;
    progressTimer.start();
    const auto updateProgress = [&](size_t sample, const QString& phase, bool force = false) {
        if (!force && progressTimer.elapsed() < 100) {
            return;
        }

        const int progress { channel.empty() ? 0 : static_cast<int>(std::min<size_t>(99, sample * 100 / static_cast<size_t>(channel.size()))) };
        setParsingProgress(true, progress, QString("Channel %1: %2").arg(chNum + 1).arg(phase));
        progressTimer.restart();
        QCoreApplication::processEvents();
    };
    QElapsedTimer parserHeartbeatTimer;
    parserHeartbeatTimer.start();
    int parserHeartbeatCounter { 0 };
    const auto updateProgressHeartbeat = [&](size_t sample, const QString& phase) {
        if (parserHeartbeatTimer.elapsed() < 300) {
            return;
        }

        const int progress { channel.empty() ? 0 : static_cast<int>(std::min<size_t>(99, sample * 100 / static_cast<size_t>(channel.size()))) };
        setParsingProgress(true,
                           progress,
                           QString("Channel %1: %2\nsample %3/%4, tick %5")
                                   .arg(chNum + 1)
                                   .arg(phase)
                                   .arg(sample)
                                   .arg(channel.size())
                                   .arg(++parserHeartbeatCounter));
        parserHeartbeatTimer.restart();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    };
    setParsingProgress(true, 0, QString("Channel %1: parsing waveform").arg(chNum + 1));
    QCoreApplication::processEvents();
    QVector<uint8_t> data;
    QVector<ParsedData::WaveformPart> waveformData;
    QMap<size_t, uint> data_mapping;
    //WaveformSign signalDirection = POSITIVE;
    size_t dataStart = 0;
    uint8_t bitIndex = 0;
    uint8_t bit = 0;
    uint8_t parity = 0;

    const auto& parserSettings = ParserSettingsModel::instance()->getParserSettings();
    auto isSineNormal = [&parserSettings, sampleRate](const ParsedData::WaveformPart& b, const ParsedData::WaveformPart& e, bool zeroCheck) -> bool {
        if (parserSettings.checkForAbnormalSine) {
            return isFreqFitsInDelta(sampleRate, b.length, zeroCheck ? parserSettings.zeroHalfFreq : parserSettings.oneHalfFreq, zeroCheck ? parserSettings.zeroDelta : parserSettings.oneDelta, parserSettings.sineCheckTolerance) &&
                   isFreqFitsInDelta(sampleRate, e.length, zeroCheck ? parserSettings.zeroHalfFreq : parserSettings.oneHalfFreq, zeroCheck ? parserSettings.zeroDelta : parserSettings.oneDelta, parserSettings.sineCheckTolerance);
        }
        return true;
    };

    const auto isPilotHalfFreq = [&parserSettings, sampleRate](const ParsedData::WaveformPart& p) -> bool {
        return isFreqFitsInDelta(sampleRate, p.length, parserSettings.pilotHalfFreq, parserSettings.pilotDelta, 1.0);
    };
    const auto isSynchroFirstHalfFreq = [&parserSettings, sampleRate](const ParsedData::WaveformPart& p, double deltaDivider = 1.0) -> bool {
        return isFreqFitsInDelta(sampleRate, p.length, parserSettings.synchroFirstHalfFreq, parserSettings.synchroDelta, deltaDivider);
    };
    const auto isSynchroSecondHalfFreq = [&parserSettings, sampleRate](const ParsedData::WaveformPart& p, double deltaDivider = 1.0) -> bool {
        return isFreqFitsInDelta(sampleRate, p.length, parserSettings.synchroSecondHalfFreq, parserSettings.synchroDelta, deltaDivider);
    };
    QElapsedTimer experimentalDebugTimer;
    experimentalDebugTimer.start();
    const auto publishExperimentalParseDebug = [&](size_t sample, const QVariantMap& periodCandidate, bool force = false) {
        if (parserSettings.parserMode != ParserSettingsModel::ExperimentalAdaptiveParser) {
            return;
        }
        if (m_experimentalDebugManualInspection) {
            return;
        }
        if (!force && experimentalDebugTimer.elapsed() < 120) {
            return;
        }

        const int bit { periodCandidate.contains("bit") ? periodCandidate.value("bit").toInt() : -1 };
        const bool valid { periodCandidate.value("valid").toBool() };
        m_experimentalDebugChannel = chNum;
        m_experimentalDebugActive = true;
        m_experimentalDebugSample = sample;
        const QString detector { periodCandidate.value("detector").toString() };
        const QString detectorLine {
            detector == "next pilot"
                    ? QString("next pilot half-waves=%1").arg(periodCandidate.value("pilotHalfWaves").toInt())
                            : detector == "pause"
                                    ? QString("pause detector rms=%1 rms ratio=%2")
                                            .arg(periodCandidate.value("rms").toDouble(), 0, 'f', 4)
                                            .arg(periodCandidate.value("rmsRatio").toDouble(), 0, 'f', 3)
                            : detector == "adaptive scan"
                                    ? QString("adaptive scan checked=%1 depth=%2 skip=%3")
                                            .arg(periodCandidate.value("checkedHypotheses").toInt())
                                            .arg(periodCandidate.value("pathDepth").toInt())
                                            .arg(periodCandidate.value("skippedHalfWaves").toInt())
                            : detector == "half-wave pair"
                                    ? QString("half-wave pair detector")
                                    : detector == "half-wave viterbi"
                                            ? QString("half-wave viterbi path=%1 depth=%2 skip=%3 score=%4 confidence=%5")
                                                    .arg(periodCandidate.value("pathBits").toString())
                                                    .arg(periodCandidate.value("pathDepth").toInt())
                                                    .arg(periodCandidate.value("skippedHalfWaves").toInt())
                                                    .arg(periodCandidate.value("pathScore").toDouble(), 0, 'f', 3)
                                                    .arg(periodCandidate.value("pathConfidence").toDouble(), 0, 'f', 3)
                                            : QString("period detector")
        };
        m_experimentalDebugState = {
            { "active", true },
            { "chNum", static_cast<int>(chNum) },
            { "sample", static_cast<int>(sample) },
            { "followSample", periodCandidate.value("followSample", true).toBool() },
            { "phase", QString("Live experimental parse") },
            { "zero", bit == 0 ? periodCandidate : QVariantMap() },
            { "one", bit == 1 ? periodCandidate : QVariantMap() },
            { "period", periodCandidate },
            { "selected", valid ? periodCandidate : QVariantMap() },
            { "result", periodCandidate.value("crcMismatchStop").toBool()
                    ? QString("Parser stopped on CRC mismatch")
                    : detector == "next pilot"
                    ? QString("Payload stopped before next pilot")
                    : detector == "pause"
                            ? QString("Payload stopped on pause")
                            : (valid ? QString("Candidate %1").arg(bit) : QString("No confident period")) },
            { "message", QString("Live experimental parse\n%1\nbit=%2 score=%3 raw=%4 penalty=%5 zero=%6 one=%7 timing=%8 balance=%9 shape=%10 speed=%11 damaged=%12 shape-rescue=%13 zero-rescue=%14\nhalves=%15/%16 begin=%17 end=%18 len=%19 axis=%20 range=%21\ncarrier ready=%22 range=%23 ratio=%24 score floor=%25 axis jump=%26/%27\ncrc stop=%28 calculated=0x%29 awaited=0x%30 bytes=%31\nsample=%32")
                    .arg(detectorLine)
                    .arg(bit)
                    .arg(periodCandidate.value("score").toDouble(), 0, 'f', 3)
                    .arg(periodCandidate.value("rawScore").toDouble(), 0, 'f', 3)
                    .arg(periodCandidate.value("totalPenalty").toDouble(), 0, 'f', 3)
                    .arg(periodCandidate.value("zeroScore").toDouble(), 0, 'f', 3)
                    .arg(periodCandidate.value("oneScore").toDouble(), 0, 'f', 3)
                    .arg(periodCandidate.value("timingScore").toDouble(), 0, 'f', 3)
                    .arg(periodCandidate.value("balanceScore").toDouble(), 0, 'f', 3)
                    .arg(periodCandidate.value("shapeScore").toDouble(), 0, 'f', 3)
                    .arg(periodCandidate.value("speedRatio").toDouble(), 0, 'f', 3)
                    .arg(periodCandidate.value("damagedReadable").toBool() ? QString("yes") : QString("no"))
                    .arg(periodCandidate.value("shapeReadable").toBool() ? QString("yes") : QString("no"))
                    .arg(periodCandidate.value("damagedZeroTimingRescue").toBool() ? QString("yes") : QString("no"))
                    .arg(periodCandidate.value("firstHalfLength").toInt())
                    .arg(periodCandidate.value("secondHalfLength").toInt())
                    .arg(periodCandidate.value("begin").toInt())
                    .arg(periodCandidate.value("end").toInt())
                    .arg(periodCandidate.value("length").toInt())
                    .arg(periodCandidate.value("axis").toDouble(), 0, 'f', 1)
                    .arg(periodCandidate.value("range").toDouble(), 0, 'f', 1)
                    .arg(periodCandidate.value("carrierReady").toBool() ? QString("yes") : QString("no"))
                    .arg(periodCandidate.value("carrierRange").toDouble(), 0, 'f', 1)
                    .arg(periodCandidate.value("rangeRatio").toDouble(), 0, 'f', 3)
                    .arg(periodCandidate.value("scoreFloor").toDouble(), 0, 'f', 3)
                    .arg(periodCandidate.value("axisJump").toDouble(), 0, 'f', 1)
                    .arg(periodCandidate.value("axisTolerance").toDouble(), 0, 'f', 1)
                    .arg(periodCandidate.value("crcMismatchStop").toBool() ? QString("yes") : QString("no"))
                    .arg(periodCandidate.value("parityCalculated").toInt(), 2, 16, QLatin1Char('0'))
                    .arg(periodCandidate.value("parityAwaited").toInt(), 2, 16, QLatin1Char('0'))
                    .arg(periodCandidate.value("parsedBytes").toInt())
                    .arg(static_cast<int>(sample)) },
        };
        emit experimentalDebugChanged(chNum);
        experimentalDebugTimer.restart();
        QCoreApplication::processEvents();
    };

    struct ExperimentalBitCandidate {
        bool valid { false };
        uint8_t bit { 0 };
        size_t begin { 0 };
        size_t end { 0 };
        double score { 0.0 };
        qsizetype firstPartIndex { -1 };
        qsizetype secondPartIndex { -1 };
    };

    const auto periodCandidateToBitCandidate = [](const QVariantMap& periodCandidate) {
        ExperimentalBitCandidate result;
        if (periodCandidate["valid"].toBool()) {
            result.valid = true;
            result.bit = static_cast<uint8_t>(periodCandidate["bit"].toInt());
            result.begin = static_cast<size_t>(periodCandidate["begin"].toInt());
            result.end = static_cast<size_t>(periodCandidate["end"].toInt());
            result.score = periodCandidate["score"].toDouble();
            result.firstPartIndex = periodCandidate.value("firstPartIndex", -1).toInt();
            result.secondPartIndex = periodCandidate.value("secondPartIndex", -1).toInt();
        }
        return result;
    };

    const auto scoreExperimentalWindow = [&channel](size_t begin, size_t length) {
        if (length < 8 || begin >= static_cast<size_t>(channel.size()) || begin + length > static_cast<size_t>(channel.size())) {
            return 0.0;
        }

        std::vector<double> samples;
        samples.reserve(length);
        for (size_t pos { begin }; pos < begin + length; ++pos) {
            samples.push_back(channel.at(static_cast<qsizetype>(pos)));
        }

        auto sortedSamples { samples };
        const auto medianIt { std::next(sortedSamples.begin(), static_cast<std::ptrdiff_t>(sortedSamples.size() / 2)) };
        std::nth_element(sortedSamples.begin(), medianIt, sortedSamples.end());
        const double axis { *medianIt };
        const auto [minIt, maxIt] { std::minmax_element(samples.cbegin(), samples.cend()) };
        const double range { *maxIt - *minIt };
        if (range <= 0.01) {
            return 0.0;
        }

        int bucketCount { std::clamp(static_cast<int>(length / 6), 8, 24) };
        bucketCount = std::min(bucketCount, static_cast<int>(length / 2));
        if (bucketCount < 4) {
            return 0.0;
        }

        QVector<int> bucketSigns;
        QVector<double> bucketWeights;
        bucketSigns.reserve(bucketCount);
        bucketWeights.reserve(bucketCount);
        const double weakBucketThreshold { range * 0.05 };
        for (int bucket { 0 }; bucket < bucketCount; ++bucket) {
            const size_t bucketBegin { length * static_cast<size_t>(bucket) / static_cast<size_t>(bucketCount) };
            const size_t bucketEnd { length * static_cast<size_t>(bucket + 1) / static_cast<size_t>(bucketCount) };
            double sum { 0.0 };
            for (size_t sample { bucketBegin }; sample < bucketEnd; ++sample) {
                sum += samples.at(sample) - axis;
            }

            const double mean { sum / std::max<size_t>(1, bucketEnd - bucketBegin) };
            bucketSigns.append(std::fabs(mean) < weakBucketThreshold ? 0 : (mean > 0.0 ? 1 : -1));
            bucketWeights.append(std::max(std::fabs(mean), weakBucketThreshold));
        }

        auto scorePolarity = [&bucketSigns, &bucketWeights, bucketCount](int split, int firstSign) {
            double matchedWeight { 0.0 };
            double totalWeight { 0.0 };
            int firstStrongBuckets { 0 };
            int secondStrongBuckets { 0 };
            for (int bucket { 0 }; bucket < bucketCount; ++bucket) {
                const int expectedSign { bucket < split ? firstSign : -firstSign };
                const int sign { bucketSigns.at(bucket) };
                const double weight { bucketWeights.at(bucket) };
                totalWeight += weight;
                if (sign == expectedSign) {
                    matchedWeight += weight;
                    if (bucket < split) {
                        ++firstStrongBuckets;
                    } else {
                        ++secondStrongBuckets;
                    }
                } else if (sign == 0) {
                    matchedWeight += weight * 0.5;
                }
            }

            if (firstStrongBuckets < split / 2 || secondStrongBuckets < (bucketCount - split) / 2) {
                return 0.0;
            }

            int signChanges { 0 };
            int previousSign { 0 };
            for (const int sign: bucketSigns) {
                if (sign == 0) {
                    continue;
                }

                if (previousSign != 0 && sign != previousSign) {
                    ++signChanges;
                }
                previousSign = sign;
            }

            const double roughnessPenalty { std::max(0, signChanges - 1) * 0.035 };
            return totalWeight > 0.0 ? std::max(0.0, matchedWeight / totalWeight - roughnessPenalty) : 0.0;
        };

        double bestScore { 0.0 };
        const int minSplit { std::max(2, static_cast<int>(std::floor(bucketCount * 0.35))) };
        const int maxSplit { std::min(bucketCount - 2, static_cast<int>(std::ceil(bucketCount * 0.65))) };
        for (int split { minSplit }; split <= maxSplit; ++split) {
            bestScore = std::max(bestScore, scorePolarity(split, 1));
            bestScore = std::max(bestScore, scorePolarity(split, -1));
        }

        double bestPlateauScore { 0.0 };
        const auto scorePlateauPolarity = [&samples, axis, range](size_t split, int firstSign) {
            double firstSum { 0.0 };
            double secondSum { 0.0 };
            int firstMatchingSamples { 0 };
            int secondMatchingSamples { 0 };
            const size_t secondSize { samples.size() - split };
            for (size_t sample { 0 }; sample < split; ++sample) {
                const double value { samples.at(sample) - axis };
                firstSum += value;
                if ((firstSign > 0 && value > 0.0) || (firstSign < 0 && value < 0.0)) {
                    ++firstMatchingSamples;
                }
            }
            for (size_t sample { split }; sample < samples.size(); ++sample) {
                const double value { samples.at(sample) - axis };
                secondSum += value;
                if ((firstSign > 0 && value < 0.0) || (firstSign < 0 && value > 0.0)) {
                    ++secondMatchingSamples;
                }
            }

            const double firstMean { firstSum / std::max<size_t>(1, split) };
            const double secondMean { secondSum / std::max<size_t>(1, secondSize) };
            if ((firstSign > 0 && (firstMean <= 0.0 || secondMean >= 0.0)) ||
                    (firstSign < 0 && (firstMean >= 0.0 || secondMean <= 0.0))) {
                return 0.0;
            }

            const double firstDominance { firstMatchingSamples / static_cast<double>(std::max<size_t>(1, split)) };
            const double secondDominance { secondMatchingSamples / static_cast<double>(std::max<size_t>(1, secondSize)) };
            const double dominanceScore { std::min(firstDominance, secondDominance) };
            const double strengthScore { std::clamp(std::min(std::fabs(firstMean), std::fabs(secondMean)) / (range * 0.18), 0.0, 1.0) };
            const double balanceScore { 1.0 - std::clamp(std::fabs(static_cast<double>(split) / samples.size() - 0.5) / 0.2, 0.0, 1.0) };
            return dominanceScore * 0.55 + strengthScore * 0.30 + balanceScore * 0.15;
        };

        const size_t minPlateauSplit { std::max<size_t>(2, static_cast<size_t>(std::floor(samples.size() * 0.35))) };
        const size_t maxPlateauSplit { std::min(samples.size() - 2, static_cast<size_t>(std::ceil(samples.size() * 0.65))) };
        for (size_t split { minPlateauSplit }; split <= maxPlateauSplit; ++split) {
            bestPlateauScore = std::max(bestPlateauScore, scorePlateauPolarity(split, 1));
            bestPlateauScore = std::max(bestPlateauScore, scorePlateauPolarity(split, -1));
        }

        const bool normalizedFloat { std::max(std::fabs(*minIt), std::fabs(*maxIt)) <= 2.0 };
        const double amplitudeScale { normalizedFloat ? 0.25 : 2500.0 };
        const double amplitudeScore { std::clamp(range / amplitudeScale, 0.0, 1.0) };
        const double shapeScore { std::max(bestScore, bestPlateauScore) };
        return std::clamp(shapeScore * 0.9 + amplitudeScore * 0.1, 0.0, 1.0);
    };

    [[maybe_unused]] const auto findExperimentalBitCandidate = [&parserSettings, sampleRate, &scoreExperimentalWindow](size_t expectedBegin, uint8_t bit) {
        const int signalFreq { bit == 0 ? parserSettings.zeroFreq : parserSettings.oneFreq };
        const double deltaBelow { bit == 0 ? parserSettings.zeroDelta : HARDCODED_DATA_SIGNAL_DELTA };
        const double deltaAbove { bit == 0 ? HARDCODED_DATA_SIGNAL_DELTA : parserSettings.oneDelta };
        const int expectedLength { static_cast<int>(std::lround(sampleRate / signalFreq)) };
        const int minLength { std::max(8, static_cast<int>(std::floor(sampleRate / (signalFreq * (1.0 + deltaAbove))))) };
        const int maxLength { std::max(minLength, static_cast<int>(std::ceil(sampleRate / (signalFreq * (1.0 - std::min(deltaBelow, 0.95)))))) };
        const int startJitter { std::max(2, expectedLength / 8) };

        ExperimentalBitCandidate best;
        best.bit = bit;
        for (int startOffset { -startJitter }; startOffset <= startJitter; ++startOffset) {
            if (startOffset < 0 && expectedBegin < static_cast<size_t>(-startOffset)) {
                continue;
            }
            const size_t begin { static_cast<size_t>(static_cast<qint64>(expectedBegin) + startOffset) };
            for (int length { minLength }; length <= maxLength; ++length) {
                double score { scoreExperimentalWindow(begin, static_cast<size_t>(length)) };
                if (score <= 0.0) {
                    continue;
                }

                const double startPenalty { std::fabs(startOffset) / static_cast<double>(std::max(1, startJitter)) * 0.08 };
                const double lengthPenalty { std::fabs(length - expectedLength) / static_cast<double>(std::max(1, expectedLength)) * (bit == 0 ? 0.22 : 0.16) };
                score = std::max(0.0, score - startPenalty - lengthPenalty);
                if (!best.valid || score > best.score) {
                    best = { true, bit, begin, begin + static_cast<size_t>(length) - 1, score };
                }
            }
        }

        return best;
    };

    const auto findNextPilotRun = [&parsed, &isPilotHalfFreq, &channel, sampleRate](size_t sample, double referenceRange) {
        struct PilotRun {
            bool found { false };
            size_t begin { 0 };
            size_t end { 0 };
            qsizetype halfWaveCount { 0 };
        };

        constexpr qsizetype c_minPilotHalfWaves { 96 };
        constexpr double c_minPauseSecondsBeforePilot { 0.008 };
        const size_t minPauseSamples { static_cast<size_t>(sampleRate * c_minPauseSecondsBeforePilot) };
        const size_t maxLookAheadSamples { static_cast<size_t>(sampleRate * 0.45) };
        auto it { std::lower_bound(parsed.begin(), parsed.end(), sample, [](const ParsedData::WaveformPart& part, size_t value) {
            return part.end < value;
        }) };
        const size_t lookAheadEnd { sample + maxLookAheadSamples };

        const auto hasPauseBeforePilot = [&channel, referenceRange, minPauseSamples, sample](size_t pilotBegin) {
            if (referenceRange <= 0.0 || pilotBegin <= sample || pilotBegin - sample < minPauseSamples) {
                return false;
            }

            const size_t windowBegin { pilotBegin - minPauseSamples };
            double minValue { channel.at(static_cast<qsizetype>(windowBegin)) };
            double maxValue { minValue };
            double sum { 0.0 };
            for (size_t pos { windowBegin }; pos < pilotBegin; ++pos) {
                const double value { channel.at(static_cast<qsizetype>(pos)) };
                minValue = std::min(minValue, value);
                maxValue = std::max(maxValue, value);
                sum += value;
            }

            const double mean { sum / static_cast<double>(minPauseSamples) };
            double squareSum { 0.0 };
            for (size_t pos { windowBegin }; pos < pilotBegin; ++pos) {
                const double diff { channel.at(static_cast<qsizetype>(pos)) - mean };
                squareSum += diff * diff;
            }

            const double rangeRatio { (maxValue - minValue) / referenceRange };
            const double rmsRatio { std::sqrt(squareSum / static_cast<double>(minPauseSamples)) / referenceRange };
            return rangeRatio < 0.30 && rmsRatio < 0.11;
        };

        while (it != parsed.end() && it->begin <= lookAheadEnd) {
            it = std::find_if(it, parsed.end(), [&isPilotHalfFreq, lookAheadEnd](const ParsedData::WaveformPart& part) {
                return part.begin <= lookAheadEnd && isPilotHalfFreq(part);
            });
            if (it == parsed.end() || it->begin > lookAheadEnd) {
                break;
            }

            const auto pilotBeginIt { it };
            auto pilotEndIt { it };
            for (; pilotEndIt != parsed.end() && isPilotHalfFreq(*pilotEndIt); ++pilotEndIt) {
            }

            const qsizetype halfWaveCount { std::distance(pilotBeginIt, pilotEndIt) };
            if (halfWaveCount >= c_minPilotHalfWaves && hasPauseBeforePilot(pilotBeginIt->begin)) {
                return PilotRun {
                    true,
                    pilotBeginIt->begin,
                    std::prev(pilotEndIt)->end,
                    halfWaveCount
                };
            }

            it = pilotEndIt;
        }

        return PilotRun {};
    };

    const auto analyzeExperimentalSignalWindow = [&channel, sampleRate](size_t sample, double referenceRange) {
        struct SignalWindow {
            bool pause { false };
            double range { 0.0 };
            double rms { 0.0 };
            double rangeRatio { 1.0 };
            double rmsRatio { 1.0 };
        };

        if (referenceRange <= 0.0 || sample >= static_cast<size_t>(channel.size())) {
            return SignalWindow {};
        }

        const size_t windowLength {
            std::min<size_t>(
                    static_cast<size_t>(channel.size()) - sample,
                    std::max<size_t>(64, static_cast<size_t>(sampleRate * 0.025)))
        };
        if (windowLength < 16) {
            return SignalWindow {};
        }

        double minValue { channel.at(static_cast<qsizetype>(sample)) };
        double maxValue { minValue };
        double sum { 0.0 };
        for (size_t offset { 0 }; offset < windowLength; ++offset) {
            const double value { channel.at(static_cast<qsizetype>(sample + offset)) };
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
            sum += value;
        }

        const double mean { sum / static_cast<double>(windowLength) };
        double squareSum { 0.0 };
        for (size_t offset { 0 }; offset < windowLength; ++offset) {
            const double value { channel.at(static_cast<qsizetype>(sample + offset)) - mean };
            squareSum += value * value;
        }

        SignalWindow result;
        result.range = maxValue - minValue;
        result.rms = std::sqrt(squareSum / static_cast<double>(windowLength));
        result.rangeRatio = result.range / referenceRange;
        result.rmsRatio = result.rms / referenceRange;
        result.pause = result.rangeRatio < 0.22 && result.rmsRatio < 0.08;
        return result;
    };

    bool stopParsingAfterExperimentalCrcError { false };
    const auto decodeExperimentalData = [&](qsizetype startPartIndex) {
        QVector<uint8_t> experimentalData;
        QVector<ParsedData::WaveformPart> experimentalWaveformData;
        QMap<size_t, uint> experimentalDataMapping;
        uint8_t experimentalBitIndex { 0 };
        uint8_t experimentalByte { 0 };
        uint8_t experimentalParity { 0 };
        qsizetype currentPartIndex { startPartIndex };
        size_t currentSample { currentPartIndex >= 0 && currentPartIndex < parsed.size() ? parsed.at(currentPartIndex).begin : 0 };
        const size_t startSample { currentSample };
        size_t lastSample { currentSample };
        ParsedData::WaveformPart lastBytePart { currentSample, currentSample, 1, ParsedData::POSITIVE };
        constexpr double c_minScore { 0.62 };
        constexpr int c_carrierWarmupBits { 16 };
        constexpr double c_minRangeRatio { 0.32 };
        constexpr double c_scoreDropTolerance { 0.24 };
        int acceptedBits { 0 };
        double carrierRange { 0.0 };
        double carrierAxis { 0.0 };
        double carrierScore { 0.0 };
        double speedRatio { 1.0 };
        size_t lastAutoSuspiciousPoint { std::numeric_limits<size_t>::max() };
        const auto calculatedParity = [&experimentalData, &experimentalParity]() {
            return experimentalData.empty() ? uint8_t { 0 } : static_cast<uint8_t>(experimentalParity ^ experimentalData.last());
        };
        const auto hasCrcMismatch = [&experimentalData, &calculatedParity]() {
            return !experimentalData.empty() && calculatedParity() != experimentalData.last();
        };
        const auto addAutoSuspiciousPoint = [&](size_t sample) {
            constexpr size_t c_minAutoSuspiciousDistance { 96 };
            if (lastAutoSuspiciousPoint != std::numeric_limits<size_t>::max() &&
                    sample > lastAutoSuspiciousPoint &&
                    sample - lastAutoSuspiciousPoint < c_minAutoSuspiciousDistance) {
                return;
            }

            if (sample <= static_cast<size_t>(std::numeric_limits<uint>::max())) {
                SuspiciousPointsModel::instance()->addSuspiciousPoint(static_cast<uint>(sample));
                lastAutoSuspiciousPoint = sample;
            }
        };

        const int zeroExpectedLength { static_cast<int>(std::lround(sampleRate / parserSettings.zeroFreq)) };
        const int oneExpectedLength { static_cast<int>(std::lround(sampleRate / parserSettings.oneFreq)) };
        const int classificationBoundary { (zeroExpectedLength + oneExpectedLength) / 2 };
        const auto scoreLength = [](double length, double expected, double deltaBelow, double deltaAbove) {
            const double minLength { expected / (1.0 + deltaAbove) };
            const double maxLength { expected / (1.0 - std::min(deltaBelow, 0.95)) };
            if (length < minLength || length > maxLength) {
                const double miss { length < minLength ? minLength - length : length - maxLength };
                return std::max(0.0, 0.45 - miss / std::max(1.0, expected) * 2.0);
            }

            return 1.0 - std::min(0.55, std::fabs(length - expected) / std::max(1.0, expected) * 1.4);
        };
        const auto buildHalfWaveCandidates = [&](qsizetype partIndex, double candidateSpeedRatio) {
            QVariantMap best {
                { "valid", false },
                { "bit", -1 },
                { "begin", partIndex >= 0 && partIndex < parsed.size() ? static_cast<int>(parsed.at(partIndex).begin) : 0 },
                { "end", partIndex >= 0 && partIndex < parsed.size() ? static_cast<int>(parsed.at(partIndex).end) : 0 },
                { "length", 0 },
                { "expectedLength", 0 },
                { "zeroExpectedLength", zeroExpectedLength },
                { "oneExpectedLength", oneExpectedLength },
                { "classificationBoundary", classificationBoundary },
                { "score", 0.0 },
                { "rawScore", 0.0 },
                { "totalPenalty", 0.0 },
                { "axis", 0.0 },
                { "upperLevel", 0.0 },
                { "lowerLevel", 0.0 },
                { "range", 0.0 },
                { "upperSample", 0 },
                { "lowerSample", 0 },
                { "detector", QString("half-wave pair") },
                { "firstPartIndex", static_cast<int>(partIndex) },
                { "secondPartIndex", static_cast<int>(partIndex + 1) },
                { "speedRatio", candidateSpeedRatio },
                { "balanceScore", 0.0 },
                { "zeroScore", 0.0 },
                { "oneScore", 0.0 },
                { "timingScore", 0.0 },
            };
            QVector<QVariantMap> rawCandidates;
            const auto appendCandidate = [&rawCandidates](const QVariantMap& candidate) {
                for (auto& existingCandidate: rawCandidates) {
                    if (existingCandidate.value("bit").toInt() == candidate.value("bit").toInt() &&
                            existingCandidate.value("secondPartIndex").toInt() == candidate.value("secondPartIndex").toInt() &&
                            existingCandidate.value("splitPartIndex").toInt() == candidate.value("splitPartIndex").toInt()) {
                        if (candidate.value("score").toDouble() > existingCandidate.value("score").toDouble()) {
                            existingCandidate = candidate;
                        }
                        return;
                    }
                }

                rawCandidates.append(candidate);
            };

            if (partIndex < 0 || partIndex + 1 >= parsed.size()) {
                return rawCandidates;
            }

            const auto dominantSign = [&parsed](qsizetype beginIndex, qsizetype endIndex) {
                size_t positiveLength { 0 };
                size_t negativeLength { 0 };
                for (qsizetype index { beginIndex }; index <= endIndex; ++index) {
                    if (parsed.at(index).sign == ParsedData::POSITIVE) {
                        positiveLength += parsed.at(index).length;
                    } else {
                        negativeLength += parsed.at(index).length;
                    }
                }
                return positiveLength >= negativeLength ? ParsedData::POSITIVE : ParsedData::NEGATIVE;
            };
            const auto spanLength = [&parsed](qsizetype beginIndex, qsizetype endIndex) {
                size_t result { 0 };
                for (qsizetype index { beginIndex }; index <= endIndex; ++index) {
                    result += parsed.at(index).length;
                }
                return result;
            };

            constexpr qsizetype c_maxPhysicalPartsPerBit { 8 };
            for (qsizetype partsConsumed { 2 }; partsConsumed <= c_maxPhysicalPartsPerBit && partIndex + partsConsumed - 1 < parsed.size(); ++partsConsumed) {
                for (qsizetype splitOffset { 1 }; splitOffset < partsConsumed; ++splitOffset) {
                    const qsizetype splitIndex { partIndex + splitOffset - 1 };
                    const qsizetype endIndex { partIndex + partsConsumed - 1 };
                    const auto& first { parsed.at(partIndex) };
                    const auto& endPart { parsed.at(endIndex) };
                    const size_t firstHalfLength { spanLength(partIndex, splitIndex) };
                    const size_t secondHalfLength { spanLength(splitIndex + 1, endIndex) };
                    const size_t length { firstHalfLength + secondHalfLength };
                    const double balanceScore {
                        1.0 - std::clamp(std::fabs(static_cast<double>(firstHalfLength) - static_cast<double>(secondHalfLength)) /
                                          static_cast<double>(std::max<size_t>(1, std::max(firstHalfLength, secondHalfLength))) / 0.55,
                                          0.0,
                                          1.0)
                    };
                    const double polarityScore { dominantSign(partIndex, splitIndex) != dominantSign(splitIndex + 1, endIndex) ? 1.0 : 0.0 };
                    const double mergePenalty { (partsConsumed - 2) * 0.055 };
                    const double splitPenalty {
                        std::abs(static_cast<double>(splitOffset) / static_cast<double>(partsConsumed) - 0.5) * 0.08
                    };
                    const double zeroScore { scoreLength(static_cast<double>(length), zeroExpectedLength * candidateSpeedRatio, parserSettings.zeroDelta, HARDCODED_DATA_SIGNAL_DELTA) };
                    const double oneScore { scoreLength(static_cast<double>(length), oneExpectedLength * candidateSpeedRatio, HARDCODED_DATA_SIGNAL_DELTA, parserSettings.oneDelta) };
                    const bool zeroHalfNormal { isSineNormal(first, endPart, true) };
                    const bool oneHalfNormal { isSineNormal(first, endPart, false) };
                    const double zeroRawScore { zeroScore * 0.70 + balanceScore * 0.22 + polarityScore * 0.08 };
                    const double oneRawScore { oneScore * 0.70 + balanceScore * 0.22 + polarityScore * 0.08 };
                    const double zeroPenalty { (zeroHalfNormal ? 0.0 : 0.10) + mergePenalty + splitPenalty };
                    const double onePenalty { (oneHalfNormal ? 0.0 : 0.10) + mergePenalty + splitPenalty };
                    const double zeroTotal { std::max(0.0, zeroRawScore - zeroPenalty) };
                    const double oneTotal { std::max(0.0, oneRawScore - onePenalty) };
                    const uint8_t bit { oneTotal > zeroTotal ? uint8_t { 1 } : uint8_t { 0 } };
                    const double rawScore { bit == 0 ? zeroRawScore : oneRawScore };
                    const double score { bit == 0 ? zeroTotal : oneTotal };
                    // Damaged tape can produce a visually clear zero with poor shape/balance score.
                    // Keep timing-strong zero candidates alive so the lookahead can decide with context.
                    const bool damagedZeroTimingRescue { bit == 0 && zeroScore >= 0.82 && polarityScore > 0.0 };
                    const double effectiveScore {
                        damagedZeroTimingRescue && score < 0.40
                                ? std::min(0.58, zeroScore * 0.62 + balanceScore * 0.18 - mergePenalty * 0.5)
                                : score
                    };
                    if (polarityScore <= 0.0 || effectiveScore <= 0.0) {
                        continue;
                    }

                    double axis { 0.0 };
                    double upperLevel { 0.0 };
                    double lowerLevel { 0.0 };
                    const double shapeScore { this->scoreExperimentalWindow(channel, first.begin, length, &axis, &upperLevel, &lowerLevel) };
                    int upperSample { static_cast<int>(first.begin) };
                    int lowerSample { static_cast<int>(first.begin) };
                    double upperValue { channel.at(static_cast<qsizetype>(first.begin)) };
                    double lowerValue { upperValue };
                    for (size_t sample { first.begin + 1 }; sample <= endPart.end; ++sample) {
                        const double value { channel.at(static_cast<qsizetype>(sample)) };
                        if (value > upperValue) {
                            upperValue = value;
                            upperSample = static_cast<int>(sample);
                        }
                        if (value < lowerValue) {
                            lowerValue = value;
                            lowerSample = static_cast<int>(sample);
                        }
                    }

                    QVariantMap candidate { best };
                    candidate["valid"] = true;
                    candidate["bit"] = bit;
                    candidate["begin"] = static_cast<int>(first.begin);
                    candidate["end"] = static_cast<int>(endPart.end);
                    candidate["length"] = static_cast<int>(length);
                    candidate["expectedLength"] = bit == 0 ? static_cast<int>(std::lround(zeroExpectedLength * candidateSpeedRatio)) : static_cast<int>(std::lround(oneExpectedLength * candidateSpeedRatio));
                    candidate["score"] = effectiveScore;
                    candidate["rawScore"] = std::max(rawScore, effectiveScore);
                    candidate["totalPenalty"] = std::max(0.0, rawScore - effectiveScore);
                    candidate["axis"] = axis;
                    candidate["upperLevel"] = upperLevel;
                    candidate["lowerLevel"] = lowerLevel;
                    candidate["range"] = upperValue - lowerValue;
                    candidate["upperSample"] = upperSample;
                    candidate["lowerSample"] = lowerSample;
                    candidate["upperPointValue"] = upperValue;
                    candidate["lowerPointValue"] = lowerValue;
                    candidate["shapeScore"] = shapeScore;
                    candidate["balanceScore"] = balanceScore;
                    candidate["zeroScore"] = zeroTotal;
                    candidate["oneScore"] = oneTotal;
                    candidate["timingScore"] = bit == 0 ? zeroScore : oneScore;
                    candidate["firstHalfLength"] = static_cast<int>(firstHalfLength);
                    candidate["secondHalfLength"] = static_cast<int>(secondHalfLength);
                    candidate["zeroHalfNormal"] = zeroHalfNormal;
                    candidate["oneHalfNormal"] = oneHalfNormal;
                    candidate["firstPartIndex"] = static_cast<int>(partIndex);
                    candidate["secondPartIndex"] = static_cast<int>(endIndex);
                    candidate["splitPartIndex"] = static_cast<int>(splitIndex);
                    candidate["physicalParts"] = static_cast<int>(partsConsumed);
                    candidate["mergePenalty"] = mergePenalty;
                    candidate["splitPenalty"] = splitPenalty;
                    candidate["damagedZeroTimingRescue"] = damagedZeroTimingRescue && effectiveScore > score;
                    appendCandidate(candidate);
                }
            }
            constexpr qsizetype c_maxAlternatives { 8 };
            return ExperimentalAdaptiveParser::selectAlternatives(rawCandidates,
                                                                  parserSettings.adaptiveAlternativeMode,
                                                                  c_maxAlternatives);
        };
        const auto buildHalfWaveCandidate = [&](qsizetype partIndex, double candidateSpeedRatio) {
            const auto candidates { buildHalfWaveCandidates(partIndex, candidateSpeedRatio) };
            if (!candidates.empty()) {
                return candidates.first();
            }

            return QVariantMap {
                { "valid", false },
                { "bit", -1 },
                { "begin", partIndex >= 0 && partIndex < parsed.size() ? static_cast<int>(parsed.at(partIndex).begin) : 0 },
                { "end", partIndex >= 0 && partIndex < parsed.size() ? static_cast<int>(parsed.at(partIndex).end) : 0 },
                { "length", 0 },
                { "expectedLength", 0 },
                { "zeroExpectedLength", zeroExpectedLength },
                { "oneExpectedLength", oneExpectedLength },
                { "classificationBoundary", classificationBoundary },
                { "score", 0.0 },
                { "rawScore", 0.0 },
                { "totalPenalty", 0.0 },
                { "axis", 0.0 },
                { "upperLevel", 0.0 },
                { "lowerLevel", 0.0 },
                { "range", 0.0 },
                { "upperSample", 0 },
                { "lowerSample", 0 },
                { "detector", QString("half-wave pair") },
                { "firstPartIndex", static_cast<int>(partIndex) },
                { "secondPartIndex", static_cast<int>(partIndex + 1) },
                { "speedRatio", candidateSpeedRatio },
                { "balanceScore", 0.0 },
                { "zeroScore", 0.0 },
                { "oneScore", 0.0 },
                { "timingScore", 0.0 },
            };
        };

        const auto selectHalfWavePath = [&](qsizetype partIndex, double initialSpeedRatio) {
            struct Path {
                bool valid { false };
                QVariantMap firstCandidate;
                double score { 0.0 };
                double confidence { 0.0 };
                QString bits;
                int depth { 0 };
                int skippedHalfWaves { 0 };
            };

            constexpr int c_maxSkippedHalfWaves { 3 };
            constexpr double c_minCandidateScore { 0.36 };
            constexpr double c_futureWeight { 0.88 };
            constexpr double c_skipPenalty { 0.52 };
            const int baseDepth { std::clamp(parserSettings.adaptiveBaseDepth, 2, 128) };
            const int uncertainDepth { std::clamp(parserSettings.adaptiveUncertainDepth, baseDepth, 128) };
            const int maxDepth { std::clamp(parserSettings.adaptiveMaxDepth, uncertainDepth, 128) };
            const int beamWidth { std::clamp(parserSettings.adaptiveBeamWidth, 2, 64) };
            const double timingStabilityPenalty { std::clamp(parserSettings.adaptiveTimingStabilityPenalty, 0.0, 2.0) };
            const auto firstCandidateProbe { buildHalfWaveCandidate(partIndex, initialSpeedRatio) };
            const double firstProbeScore { firstCandidateProbe.value("score").toDouble() };
            const double firstProbeTimingScore { firstCandidateProbe.value("timingScore").toDouble() };
            const bool firstProbeRescued { firstCandidateProbe.value("damagedZeroTimingRescue").toBool() };
            const int pathDepth {
                !firstCandidateProbe.value("valid").toBool() || firstProbeScore < 0.50
                        ? maxDepth
                        : (firstProbeScore < 0.74 || firstProbeTimingScore < 0.86 || firstProbeRescued ? uncertainDepth : baseDepth)
            };

            struct BeamState {
                qsizetype index { 0 };
                double speedRatio { 1.0 };
                double score { 0.0 };
                double weight { 0.0 };
                QVariantMap firstCandidate;
                QString bits;
                int depth { 0 };
                int skippedHalfWaves { 0 };
            };

            QVector<BeamState> beam {
                BeamState {
                    partIndex,
                    initialSpeedRatio,
                    0.0,
                    0.0,
                    {},
                    {},
                    0,
                    0
                }
            };
            Path path;
            std::atomic<int> checkedHypotheses { 0 };

            for (int depth { 0 }; depth < pathDepth && !beam.empty(); ++depth) {
                if (m_parsingCancellationRequested) {
                    return Path {};
                }

                const auto frontIndex { beam.first().index };
                const size_t statusSample { frontIndex >= 0 && frontIndex < parsed.size() ? parsed.at(frontIndex).begin : currentSample };
                updateProgressHeartbeat(statusSample,
                                        QString("adaptive beam, accepted bits %1, depth %2/%3, states %4, checked %5")
                                                .arg(acceptedBits)
                                                .arg(depth + 1)
                                                .arg(pathDepth)
                                                .arg(beam.size())
                                                .arg(checkedHypotheses.load()));
                if (frontIndex >= 0 && frontIndex < parsed.size()) {
                    const qsizetype endIndex { std::min<qsizetype>(parsed.size() - 1, frontIndex + 4) };
                    QVariantMap scanCandidate {
                        { "valid", true },
                        { "bit", -1 },
                        { "begin", static_cast<int>(parsed.at(frontIndex).begin) },
                        { "end", static_cast<int>(parsed.at(endIndex).end) },
                        { "length", static_cast<int>(parsed.at(endIndex).end - parsed.at(frontIndex).begin + 1) },
                        { "score", 0.0 },
                        { "rawScore", 0.0 },
                        { "totalPenalty", 0.0 },
                        { "axis", 0.0 },
                        { "upperLevel", 0.0 },
                        { "lowerLevel", 0.0 },
                        { "range", 0.0 },
                        { "upperSample", static_cast<int>(parsed.at(frontIndex).begin) },
                        { "lowerSample", static_cast<int>(parsed.at(endIndex).end) },
                        { "upperPointValue", 0.0 },
                        { "lowerPointValue", 0.0 },
                        { "detector", QString("adaptive scan") },
                        { "followSample", false },
                        { "checkedHypotheses", checkedHypotheses.load() },
                        { "pathDepth", depth + 1 },
                        { "skippedHalfWaves", beam.first().skippedHalfWaves },
                    };
                    publishExperimentalParseDebug(statusSample, scanCandidate);
                }

                QVector<BeamState> nextBeam;
                nextBeam.reserve(beamWidth * 2);
                const auto buildSuccessors = [&](const BeamState& state) {
                        QVector<BeamState> successors;
                        if (state.index + 1 >= parsed.size()) {
                            return successors;
                        }

                        const auto candidates { buildHalfWaveCandidates(state.index, state.speedRatio) };
                        checkedHypotheses.fetch_add(static_cast<int>(candidates.size()));
                        for (auto candidate: candidates) {
                            const double candidateScore { candidate.value("score").toDouble() };
                            if (!candidate.value("valid").toBool() || candidateScore < c_minCandidateScore) {
                                continue;
                            }

                            const uint8_t bit { static_cast<uint8_t>(candidate.value("bit").toInt()) };
                            const double expectedLength { static_cast<double>(bit == 0 ? zeroExpectedLength : oneExpectedLength) };
                            const double observedSpeedRatio { candidate.value("length").toDouble() / std::max(1.0, expectedLength) };
                            const double nextSpeedRatio { std::clamp(state.speedRatio * 0.86 + observedSpeedRatio * 0.14, 0.70, 1.35) };
                            const double speedJumpPenalty {
                                timingStabilityPenalty *
                                std::clamp(std::fabs(std::log(std::max(0.01, observedSpeedRatio) / std::max(0.01, state.speedRatio))) / 0.18,
                                           0.0,
                                           1.0)
                            };
                            const double transitionBonus {
                                state.bits.endsWith("0") && bit == 1 ? 0.06 :
                                state.bits.endsWith("1") && bit == 0 ? 0.03 :
                                0.0
                            };
                            candidate["speedJumpPenalty"] = speedJumpPenalty;
                            successors.append(BeamState {
                                static_cast<qsizetype>(candidate.value("secondPartIndex").toInt()) + 1,
                                nextSpeedRatio,
                                state.score + (candidateScore + transitionBonus - speedJumpPenalty) * std::pow(c_futureWeight, state.depth),
                                state.weight + std::pow(c_futureWeight, state.depth),
                                state.firstCandidate.empty() ? candidate : state.firstCandidate,
                                state.bits + QString::number(bit),
                                state.depth + 1,
                                state.skippedHalfWaves
                            });
                        }

                        if (state.skippedHalfWaves < c_maxSkippedHalfWaves && state.index + 1 < parsed.size()) {
                            successors.append(BeamState {
                                state.index + 1,
                                state.speedRatio,
                                state.score - c_skipPenalty,
                                state.weight,
                                state.firstCandidate,
                                state.bits,
                                state.depth,
                                state.skippedHalfWaves + 1
                            });
                        }

                        return successors;
                };

                if (ExperimentalAdaptiveParser::shouldRunBeamInParallel(beam.size())) {
                    std::vector<std::future<QVector<BeamState>>> futures;
                    futures.reserve(static_cast<size_t>(beam.size()));
                    for (const auto& state: beam) {
                        futures.push_back(std::async(std::launch::async, buildSuccessors, state));
                    }

                    for (auto& future: futures) {
                        const auto successors { future.get() };
                        for (const auto& state: successors) {
                            nextBeam.append(state);
                        }
                    }
                } else {
                    for (const auto& state: beam) {
                        const auto successors { buildSuccessors(state) };
                        for (const auto& successor: successors) {
                            nextBeam.append(successor);
                        }
                    }
                }

                std::sort(nextBeam.begin(), nextBeam.end(), [](const BeamState& left, const BeamState& right) {
                    return left.score > right.score;
                });
                if (nextBeam.size() > beamWidth) {
                    nextBeam.resize(beamWidth);
                }

                for (const auto& state: nextBeam) {
                    if (!state.firstCandidate.empty() && (!path.valid || state.score > path.score)) {
                        path.valid = true;
                        path.firstCandidate = state.firstCandidate;
                        path.score = state.score;
                        path.confidence = state.weight > 0.0 ? state.score / state.weight : 0.0;
                        path.bits = state.bits;
                        path.depth = state.depth;
                        path.skippedHalfWaves = state.skippedHalfWaves;
                    }
                }
                beam = std::move(nextBeam);
            }
            if (path.valid) {
                path.firstCandidate["detector"] = QString("half-wave viterbi");
                path.firstCandidate["pathScore"] = path.score;
                path.firstCandidate["pathConfidence"] = path.confidence;
                path.firstCandidate["pathBits"] = path.bits;
                path.firstCandidate["pathDepth"] = path.depth;
                path.firstCandidate["skippedHalfWaves"] = path.skippedHalfWaves;
            }
            return path;
        };

        while (currentPartIndex + 1 < parsed.size() && currentSample < static_cast<size_t>(channel.size())) {
            if (m_parsingCancellationRequested) {
                break;
            }

            updateProgress(currentSample,
                           QString("experimental adaptive: accepted bits %1, part %2/%3")
                                   .arg(acceptedBits)
                                   .arg(currentPartIndex)
                                   .arg(parsed.size()));
            const bool carrierReadyForPause { acceptedBits >= c_carrierWarmupBits && carrierRange > 0.0 };
            const auto signalWindow { analyzeExperimentalSignalWindow(currentSample, carrierRange) };
            if (carrierReadyForPause && signalWindow.pause) {
                QVariantMap stopCandidate {
                    { "valid", false },
                    { "bit", -1 },
                    { "begin", static_cast<int>(currentSample) },
                    { "end", static_cast<int>(currentSample) },
                    { "length", 0 },
                    { "score", 0.0 },
                    { "rawScore", 0.0 },
                    { "totalPenalty", 0.0 },
                    { "axis", 0.0 },
                    { "range", signalWindow.range },
                    { "carrierReady", carrierReadyForPause },
                    { "carrierRange", carrierRange },
                    { "carrierAxis", carrierAxis },
                    { "carrierScore", carrierScore },
                    { "rangeRatio", signalWindow.rangeRatio },
                    { "scoreFloor", 0.0 },
                    { "axisJump", 0.0 },
                    { "axisTolerance", 0.0 },
                    { "detector", QString("pause") },
                    { "rms", signalWindow.rms },
                    { "rmsRatio", signalWindow.rmsRatio },
                };
                publishExperimentalParseDebug(currentSample, stopCandidate, true);
                break;
            }

            const auto nextPilotRun { findNextPilotRun(currentSample, carrierRange) };
            if (nextPilotRun.found) {
                QVariantMap stopCandidate {
                    { "valid", false },
                    { "bit", -1 },
                    { "begin", static_cast<int>(nextPilotRun.begin) },
                    { "end", static_cast<int>(nextPilotRun.end) },
                    { "length", static_cast<int>(nextPilotRun.end - nextPilotRun.begin + 1) },
                    { "score", 0.0 },
                    { "rawScore", 0.0 },
                    { "totalPenalty", 0.0 },
                    { "axis", 0.0 },
                    { "range", 0.0 },
                    { "carrierReady", acceptedBits >= c_carrierWarmupBits },
                    { "carrierRange", carrierRange },
                    { "carrierAxis", carrierAxis },
                    { "carrierScore", carrierScore },
                    { "rangeRatio", 0.0 },
                    { "scoreFloor", 0.0 },
                    { "axisJump", 0.0 },
                    { "axisTolerance", 0.0 },
                    { "detector", QString("next pilot") },
                    { "pilotHalfWaves", static_cast<int>(nextPilotRun.halfWaveCount) },
                };
                publishExperimentalParseDebug(currentSample, stopCandidate, true);
                break;
            }

            auto path { selectHalfWavePath(currentPartIndex, speedRatio) };
            auto periodCandidateMap {
                path.valid && !path.firstCandidate.empty()
                        ? path.firstCandidate
                        : buildHalfWaveCandidate(currentPartIndex, speedRatio)
            };
            const double candidateRange { periodCandidateMap.value("range").toDouble() };
            const double candidateAxis { periodCandidateMap.value("axis").toDouble() };
            const double candidateScore { periodCandidateMap.value("score").toDouble() };
            const bool carrierReady { acceptedBits >= c_carrierWarmupBits && carrierRange > 0.0 };
            const double rangeRatio { carrierReady ? candidateRange / carrierRange : 1.0 };
            const double scoreFloor { carrierReady ? std::max(c_minScore, carrierScore - c_scoreDropTolerance) : c_minScore };
            const double axisJump { carrierReady ? std::fabs(candidateAxis - carrierAxis) : 0.0 };
            const double axisTolerance { carrierReady ? std::max(carrierRange * 2.0, candidateRange * 3.0) : std::numeric_limits<double>::max() };
            const double pathConfidence { periodCandidateMap.value("pathConfidence").toDouble() };
            const double timingScore { periodCandidateMap.value("timingScore").toDouble() };
            const double balanceScore { periodCandidateMap.value("balanceScore").toDouble() };
            const double shapeScore { periodCandidateMap.value("shapeScore").toDouble() };
            const bool shapeReadable {
                periodCandidateMap.value("valid").toBool() &&
                shapeScore >= 0.70 &&
                balanceScore >= 0.58 &&
                pathConfidence >= 0.45 &&
                timingScore >= 0.32 &&
                (!carrierReady || rangeRatio >= 0.10) &&
                axisJump <= axisTolerance
            };
            const bool damagedButReadable {
                periodCandidateMap.value("valid").toBool() &&
                timingScore >= 0.76 &&
                pathConfidence >= 0.62 &&
                (!carrierReady || rangeRatio >= 0.10) &&
                axisJump <= axisTolerance
            };
            const bool periodAccepted {
                periodCandidateMap.value("valid").toBool() &&
                (candidateScore >= c_minScore ||
                 (candidateScore >= 0.48 && pathConfidence >= 0.66) ||
                 damagedButReadable ||
                 shapeReadable)
            };
            const bool carrierAccepted {
                !carrierReady ||
                damagedButReadable ||
                shapeReadable ||
                ((rangeRatio >= c_minRangeRatio || candidateScore >= 0.86) &&
                 candidateScore >= scoreFloor &&
                 axisJump <= axisTolerance)
            };
            periodCandidateMap["carrierReady"] = carrierReady;
            periodCandidateMap["carrierRange"] = carrierRange;
            periodCandidateMap["carrierAxis"] = carrierAxis;
            periodCandidateMap["carrierScore"] = carrierScore;
            periodCandidateMap["rangeRatio"] = rangeRatio;
            periodCandidateMap["scoreFloor"] = scoreFloor;
            periodCandidateMap["axisJump"] = axisJump;
            periodCandidateMap["axisTolerance"] = axisTolerance;
            periodCandidateMap["damagedReadable"] = damagedButReadable;
            periodCandidateMap["shapeReadable"] = shapeReadable;
            periodCandidateMap["balanceScore"] = balanceScore;
            publishExperimentalParseDebug(currentSample, periodCandidateMap);
            const auto periodCandidate { periodCandidateToBitCandidate(periodCandidateMap) };
            ExperimentalBitCandidate candidate;
            if (periodCandidate.valid && periodAccepted && carrierAccepted) {
                candidate = periodCandidate;
                if (damagedButReadable || shapeReadable || candidateScore < c_minScore || pathConfidence < 0.70) {
                    addAutoSuspiciousPoint(candidate.begin);
                }
            } else {
                const int suspiciousBegin { periodCandidateMap.value("begin", static_cast<int>(currentSample)).toInt() };
                if (suspiciousBegin >= 0 &&
                        (periodCandidateMap.value("valid").toBool() ||
                         timingScore >= 0.60 ||
                         pathConfidence >= 0.50)) {
                    addAutoSuspiciousPoint(static_cast<size_t>(suspiciousBegin));
                }
                if (hasCrcMismatch()) {
                    stopParsingAfterExperimentalCrcError = true;
                    periodCandidateMap["crcMismatchStop"] = true;
                    periodCandidateMap["parityCalculated"] = static_cast<int>(calculatedParity());
                    periodCandidateMap["parityAwaited"] = static_cast<int>(experimentalData.last());
                    periodCandidateMap["parsedBytes"] = experimentalData.size();
                    publishExperimentalParseDebug(currentSample, periodCandidateMap, true);
                }
                break;
            }

            if (candidate.secondPartIndex < currentPartIndex) {
                periodCandidateMap["detector"] = QString("parser guard");
                periodCandidateMap["valid"] = false;
                periodCandidateMap["score"] = candidate.score;
                publishExperimentalParseDebug(currentSample, periodCandidateMap, true);
                break;
            }

            const ParsedData::WaveformPart bitPart {
                candidate.begin,
                candidate.end,
                candidate.end - candidate.begin + 1,
                lessThanZero(channel.at(static_cast<qsizetype>(candidate.begin))) ? ParsedData::NEGATIVE : ParsedData::POSITIVE
            };

            const uint8_t waveformFlag { candidate.bit == 0 ? ParsedData::zeroBit : ParsedData::oneBit };
            parsedData.fillParsedWaveform(bitPart,
                                          waveformFlag | ParsedData::sequenceMiddle,
                                          bitPart.begin,
                                          waveformFlag | ParsedData::sequenceBegin | (experimentalBitIndex == 0 ? ParsedData::byteBound : 0),
                                          bitPart.end,
                                          waveformFlag | ParsedData::sequenceEnd | (experimentalBitIndex == 7 ? ParsedData::byteBound : 0));

            if (experimentalBitIndex == 0) {
                experimentalDataMapping.insert(bitPart.begin, experimentalData.size());
            }

            if (candidate.bit != 0) {
                experimentalByte |= 1 << (7 - experimentalBitIndex);
            }
            lastBytePart = bitPart;
            lastSample = bitPart.end;
            currentPartIndex = candidate.secondPartIndex + 1;
            currentSample = currentPartIndex >= 0 && currentPartIndex < parsed.size() ? parsed.at(currentPartIndex).begin : bitPart.end + 1;
            if (carrierRange <= 0.0) {
                carrierRange = candidateRange;
                carrierAxis = candidateAxis;
                carrierScore = candidateScore;
            } else {
                const double alpha { acceptedBits < c_carrierWarmupBits ? 1.0 / static_cast<double>(acceptedBits + 1) : 0.08 };
                carrierRange = carrierRange * (1.0 - alpha) + candidateRange * alpha;
                carrierAxis = carrierAxis * (1.0 - alpha) + candidateAxis * alpha;
                carrierScore = carrierScore * (1.0 - alpha) + candidateScore * alpha;
            }
            const double bitExpectedLength { static_cast<double>(candidate.bit == 0 ? zeroExpectedLength : oneExpectedLength) };
            const double observedSpeedRatio { static_cast<double>(bitPart.length) / std::max(1.0, bitExpectedLength) };
            const double speedAlpha { acceptedBits < c_carrierWarmupBits ? 1.0 / static_cast<double>(acceptedBits + 1) : 0.05 };
            speedRatio = std::clamp(speedRatio * (1.0 - speedAlpha) + observedSpeedRatio * speedAlpha, 0.70, 1.35);
            ++acceptedBits;

            if (experimentalBitIndex++ == 7) {
                experimentalDataMapping.insert(bitPart.end, experimentalData.size());
                experimentalBitIndex = 0;
                experimentalData.append(experimentalByte);
                experimentalWaveformData.append(lastBytePart);
                experimentalParity ^= experimentalByte;
                experimentalByte = 0;
            }
        }

        if (!experimentalData.empty()) {
            experimentalParity ^= experimentalData.last();
            parsedData.storeData(std::move(experimentalData), std::move(experimentalDataMapping), startSample, lastSample, std::move(experimentalWaveformData), experimentalParity);
        }

        return lastSample;
    };

    parsedData.beginParse();

    while (currentState != NO_MORE_DATA) {
        if (m_parsingCancellationRequested) {
            break;
        }

        auto prevIt = it;
        const size_t progressSample {
            it != parsed.end() ? it->begin :
            prevIt != parsed.end() ? prevIt->end :
            static_cast<size_t>(channel.size())
        };
        updateProgress(progressSample, QString("standard states"));
        switch (currentState) {
        case SEARCH_OF_PILOT_TONE:
            it = std::find_if(it, parsed.end(), [&isPilotHalfFreq, &parsedData](const ParsedData::WaveformPart& p) {
                parsedData.fillParsedWaveform(p, 0);
                return isPilotHalfFreq(p);
            });
            if (it != parsed.end()) {
                currentState = PILOT_TONE;
            }
            break;

        case PILOT_TONE:
            for (; it != parsed.end() && isPilotHalfFreq(*it); ++it) {
                //Mark parsed waveform as pilot-tone
                parsedData.fillParsedWaveform(*it, ParsedData::pilotTone | ParsedData::sequenceMiddle);
            };

            //Found the first half of SYNCHRO signal
            if (it != parsed.end()) {
                if (auto itnext { std::next(it) };
                    (parserSettings.preciseSynchroCheck && isSynchroFirstHalfFreq(*it)) ||
                    (!parserSettings.preciseSynchroCheck &&
                     (itnext != parsed.end() &&
                      isFreqFitsInDelta(sampleRate, it->length + itnext->length, parserSettings.synchroFreq, parserSettings.synchroDelta, 1.0))))
                {
                    auto eIt = std::prev(it);
                    //Mark parsed waveform as pilot-tone and sets the begin and end bounds
                    parsedData.fillParsedWaveform(*eIt, ParsedData::pilotTone | ParsedData::sequenceMiddle,
                                                  prevIt->begin, ParsedData::pilotTone | ParsedData::sequenceBegin,
                                                  eIt->end, ParsedData::pilotTone | ParsedData::sequenceEnd);
                    currentState = SYNCHRO_SIGNAL;
                }
                else {
                    currentState = SEARCH_OF_PILOT_TONE;
                }
            }
            else {
                currentState = SEARCH_OF_PILOT_TONE;
            }
            break;

        case SYNCHRO_SIGNAL:
            it = std::next(it);
            if (it != parsed.end()) {
                //Check for second half of SYNCHRO signal or if `preciseSynchroCheck` option is off - assume there is synchro, because we did the check on the previous step
                if (!parserSettings.preciseSynchroCheck || isSynchroSecondHalfFreq(*it)) {
                    //Mark parsed waveform as syncro signal and sets the begin and end bounds
                    parsedData.fillParsedWaveform(*prevIt, *it, ParsedData::synchroSignal | ParsedData::sequenceMiddle,
                                                  ParsedData::synchroSignal | ParsedData::sequenceBegin,
                                                  ParsedData::synchroSignal | ParsedData::sequenceEnd);

                    //Initializing the currently parsing data block
                    currentState = DATA_SIGNAL;

                    it = std::next(it);
                    dataStart = std::distance(parsed.begin(), it);
                    data.clear();
                    waveformData.clear();
                    data_mapping.clear();
                    bitIndex ^= bitIndex;
                    bit ^= bit;
                }
                else {
                    //Got the synchro error
                    currentState = SEARCH_OF_PILOT_TONE;
                }
            }
            break;

        case DATA_SIGNAL:
            if (parserSettings.parserMode == ParserSettingsModel::ExperimentalAdaptiveParser) {
                const qsizetype experimentalStartPart { it != parsed.end() ? std::distance(parsed.begin(), it) : std::distance(parsed.begin(), prevIt) + 1 };
                const size_t endSample { decodeExperimentalData(experimentalStartPart) };
                it = std::find_if(it, parsed.end(), [endSample](const ParsedData::WaveformPart& p) {
                    return p.begin > endSample;
                });
                currentState = stopParsingAfterExperimentalCrcError ? NO_MORE_DATA : END_OF_DATA;
                break;
            }

            it = std::next(it);
            if (const auto storeParsedData = [&](size_t parsedBegin, size_t parsedEnd) {
                    if (!data.empty()) {
                        parity ^= data.last(); //Removing parity byte from overal parity check sum
                        //Storing parsed data
                        parsedData.storeData(std::move(data), std::move(data_mapping), parsed.at(parsedBegin).begin, parsed.at(parsedEnd).end, std::move(waveformData), parity);
                        parity ^= parity; //Zeroing parity byte
                    }
                };
                it != parsed.end())
            {
                const auto len = it->length + prevIt->length;
                const auto storeParsedByte = [&bitIndex, &data, &waveformData, &parity, &bit, it]() {
                    //Store parsed byte in data buffer
                    bitIndex ^= bitIndex;
                    data.append(bit);
                    waveformData.append(*it);
                    parity ^= bit;
                    bit ^= bit;
                };

                //"0" - ZERO
                if (isZeroFreqFitsInDelta(sampleRate, len, parserSettings.zeroFreq, parserSettings.zeroDelta, HARDCODED_DATA_SIGNAL_DELTA) && isSineNormal(*prevIt, *it, true)) {
                    //Mark parsed waveform as "0"-bit and sets the begin and end bounds
                    parsedData.fillParsedWaveform(*prevIt, *it, ParsedData::zeroBit | ParsedData::sequenceMiddle,
                                                  ParsedData::zeroBit | ParsedData::sequenceBegin | (bitIndex == 0 ? ParsedData::byteBound : 0),
                                                  ParsedData::zeroBit | ParsedData::sequenceEnd | (bitIndex == 7 ? ParsedData::byteBound : 0));

                    if (bitIndex == 0) {
                        data_mapping.insert((*prevIt).begin, data.size());
                    }

                    if (bitIndex++ == 7) {
                        data_mapping.insert((*it).end, data.size());
                        storeParsedByte();
                        // //Update byte bound mark
                        // parsedData.orParsedWaveform(it->end, ParsedData::byteBound);
                        // //Store parsed byte in data buffer
                        // bitIndex ^= bitIndex;
                        // data.append(bit);
                        // waveformData.append(*it);
                        // parity ^= bit;
                        // bit ^= bit;
                    }
                }
                // "1" - ONE
                else if (isOneFreqFitsInDelta(sampleRate, len, parserSettings.oneFreq, HARDCODED_DATA_SIGNAL_DELTA, parserSettings.oneDelta) && isSineNormal(*prevIt, *it, false)) {
                    //Mark parsed waveform as "1"-bit and sets the begin and end bounds
                    parsedData.fillParsedWaveform(*prevIt, *it, ParsedData::oneBit | ParsedData::sequenceMiddle,
                                                  ParsedData::oneBit | ParsedData::sequenceBegin | (bitIndex == 0 ? ParsedData::byteBound : 0),
                                                  ParsedData::oneBit | ParsedData::sequenceEnd | (bitIndex == 7 ? ParsedData::byteBound : 0));

                    if (bitIndex == 0) {
                        data_mapping.insert((*prevIt).begin, data.size());
                    }

                    //Set the currently parsed bit
                    bit |= 1 << (7 - bitIndex);
                    if (bitIndex++ == 7) {
                        data_mapping.insert((*it).end, data.size());
                        storeParsedByte();
                        // //Update byte bound mark
                        // parsedData.orParsedWaveform(it->end, ParsedData::byteBound);
                        // //Store parsed byte in data buffer
                        // bitIndex ^= bitIndex;
                        // data.append(bit);
                        // waveformData.append(*it);
                        // parity ^= bit;
                        // bit ^= bit;
                    }
                }
                else {
                    currentState = END_OF_DATA;
                    storeParsedData(dataStart, std::distance(parsed.begin(), it));
                    // if (!data.empty()) {
                    //     parity ^= data.last(); //Removing parity byte from overal parity check sum
                    //     //Storing parsed data
                    //     parsedData.storeData(std::move(data), std::move(data_mapping), parsed.at(dataStart).begin, parsed.at(std::distance(parsed.begin(), it)).end, std::move(waveformData), parity);
                    //     parity ^= parity; //Zeroing parity byte
                    // }
                }
                it = std::next(it);
            }
            else {
                storeParsedData(dataStart, parsed.size() - 1);
                // if (!data.empty()) {
                //     parity ^= data.last(); //Remove parity byte from overal parity check sum
                //     //Storing parsed data
                //     parsedData.storeData(std::move(data), std::move(data_mapping), parsed.at(dataStart).begin, parsed.at(parsed.size() - 1).end, std::move(waveformData), parity);
                //     parity ^= parity; //Zeroing parity byte
                // }
            }
            break;

        case END_OF_DATA:
            currentState = SEARCH_OF_PILOT_TONE;
            break;

        default:
            break;
        }

        if (it == parsed.end()) {
            currentState = NO_MORE_DATA;
        }
    }

    parsedData.endParse();
    setParsingProgress(true,
                       m_parsingCancellationRequested ? m_parsingProgress : 100,
                       m_parsingCancellationRequested ? QString("Channel %1: canceled").arg(chNum + 1) : QString("Channel %1: done").arg(chNum + 1));
    QCoreApplication::processEvents();

    if (chNum == 0) {
        emit parsedChannel0Changed();
    }
    else {
        emit parsedChannel1Changed();
    }
    setParsingProgress(false, 100, QString());
}

void WaveformParser::setExperimentalDebugInactive(uint chNum, const QString& message)
{
    m_experimentalDebugActive = false;
    m_experimentalDebugManualInspection = false;
    m_experimentalDebugState = {
        { "active", false },
        { "chNum", static_cast<int>(chNum) },
        { "message", message },
    };
    emit experimentalDebugChanged(chNum);
}

bool WaveformParser::startExperimentalDebug(uint chNum)
{
    if (chNum >= mWavReader.getNumberOfChannels()) {
        setExperimentalDebugInactive(chNum, "Invalid channel.");
        return false;
    }

    const auto channel { chNum == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1() };
    if (channel.isNull() || channel->empty()) {
        setExperimentalDebugInactive(chNum, "No channel data.");
        return false;
    }

    const double sampleRate { static_cast<double>(mWavReader.getSampleRate()) };
    const auto& parserSettings { ParserSettingsModel::instance()->getParserSettings() };
    const auto isPilotHalfFreq = [&parserSettings, sampleRate](const ParsedData::WaveformPart& p) {
        return isFreqFitsInDelta(sampleRate, p.length, parserSettings.pilotHalfFreq, parserSettings.pilotDelta, 1.0);
    };
    const auto isSynchroFirstHalfFreq = [&parserSettings, sampleRate](const ParsedData::WaveformPart& p, double deltaDivider = 1.0) {
        return isFreqFitsInDelta(sampleRate, p.length, parserSettings.synchroFirstHalfFreq, parserSettings.synchroDelta, deltaDivider);
    };
    const auto isSynchroSecondHalfFreq = [&parserSettings, sampleRate](const ParsedData::WaveformPart& p, double deltaDivider = 1.0) {
        return isFreqFitsInDelta(sampleRate, p.length, parserSettings.synchroSecondHalfFreq, parserSettings.synchroDelta, deltaDivider);
    };
    m_experimentalDebugParsed = parseChannel<QWavVectorType>(*channel);
    auto it { m_experimentalDebugParsed.begin() };
    while (it != m_experimentalDebugParsed.end()) {
        it = std::find_if(it, m_experimentalDebugParsed.end(), isPilotHalfFreq);
        if (it == m_experimentalDebugParsed.end()) {
            break;
        }

        for (; it != m_experimentalDebugParsed.end() && isPilotHalfFreq(*it); ++it) {
        }

        if (it == m_experimentalDebugParsed.end()) {
            break;
        }

        const auto itnext { std::next(it) };
        const bool syncFound {
            (parserSettings.preciseSynchroCheck && isSynchroFirstHalfFreq(*it)) ||
            (!parserSettings.preciseSynchroCheck &&
             (itnext != m_experimentalDebugParsed.end() &&
              isFreqFitsInDelta(sampleRate, it->length + itnext->length, parserSettings.synchroFreq, parserSettings.synchroDelta, 1.0)))
        };

        if (syncFound && itnext != m_experimentalDebugParsed.end() &&
                (!parserSettings.preciseSynchroCheck || isSynchroSecondHalfFreq(*itnext))) {
            m_experimentalDebugChannel = chNum;
            m_experimentalDebugActive = true;
            m_experimentalDebugManualInspection = false;
            m_experimentalDebugSample = itnext->end + 1;
            m_experimentalDebugState = {
                { "active", true },
                { "chNum", static_cast<int>(chNum) },
                { "sample", static_cast<int>(m_experimentalDebugSample) },
                { "message", QString("Pilot/sync found. Press Next.") },
            };
            emit experimentalDebugChanged(chNum);
            return true;
        }
    }

    setExperimentalDebugInactive(chNum, "Pilot/sync not found.");
    return false;
}

bool WaveformParser::nextExperimentalDebugStep()
{
    if (!m_experimentalDebugActive) {
        return false;
    }

    const auto channel { m_experimentalDebugChannel == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1() };
    if (channel.isNull() || channel->empty() || m_experimentalDebugSample >= static_cast<size_t>(channel->size())) {
        setExperimentalDebugInactive(m_experimentalDebugChannel, "No more samples.");
        return false;
    }

    const auto& parserSettings { ParserSettingsModel::instance()->getParserSettings() };
    const double sampleRate { static_cast<double>(mWavReader.getSampleRate()) };
    constexpr double c_minScore { 0.68 };
    const auto candidateSummary = [](const QString& name, const QVariantMap& candidate) {
        if (candidate.empty()) {
            return QString("%1: not checked on this step").arg(name);
        }

        return QString("%1: %2 bit=%3 score=%4 raw=%5 penalty=%6, ref len=%7 [%8..%9], jitter=%10\n"
                       "   found begin=%11 end=%12 len=%13, axis=%14, upper=%15@%16, lower=%17@%18")
                .arg(name)
                .arg(candidate["valid"].toBool() ? QString("valid") : QString("invalid"))
                .arg(candidate["bit"].toInt())
                .arg(candidate["score"].toDouble(), 0, 'f', 3)
                .arg(candidate["rawScore"].toDouble(), 0, 'f', 3)
                .arg(candidate["totalPenalty"].toDouble(), 0, 'f', 3)
                .arg(candidate["expectedLength"].toInt())
                .arg(candidate["minLength"].toInt())
                .arg(candidate["maxLength"].toInt())
                .arg(candidate["startJitter"].toInt())
                .arg(candidate["begin"].toInt())
                .arg(candidate["end"].toInt())
                .arg(candidate["length"].toInt())
                .arg(candidate["axis"].toDouble(), 0, 'f', 1)
                .arg(candidate["upperLevel"].toDouble(), 0, 'f', 1)
                .arg(candidate["upperSample"].toInt())
                .arg(candidate["lowerLevel"].toDouble(), 0, 'f', 1)
                .arg(candidate["lowerSample"].toInt());
    };

    QVariantMap periodCandidate { findExperimentalPeriodCandidate(*channel, m_experimentalDebugSample, parserSettings, sampleRate) };
    QVariantMap selectedCandidate;
    QString result;
    QString phase;

    const bool periodAccepted { periodCandidate["valid"].toBool() && periodCandidate["score"].toDouble() >= c_minScore };
    phase = "Step: detected complete period first, then classified by length";
    if (periodAccepted) {
        selectedCandidate = periodCandidate;
        result = QString("Selected %1").arg(periodCandidate["bit"].toInt());
    } else {
        result = QString("No confident period. score=%1")
                .arg(periodCandidate["score"].toDouble(), 0, 'f', 3);
    }

    if (!selectedCandidate.empty()) {
        m_experimentalDebugSample = static_cast<size_t>(selectedCandidate["end"].toInt()) + 1;
    }

    m_experimentalDebugManualInspection = false;
    m_experimentalDebugState = {
        { "active", true },
        { "chNum", static_cast<int>(m_experimentalDebugChannel) },
        { "sample", static_cast<int>(m_experimentalDebugSample) },
        { "phase", phase },
        { "zero", periodCandidate["bit"].toInt() == 0 ? periodCandidate : QVariantMap() },
        { "one", periodCandidate["bit"].toInt() == 1 ? periodCandidate : QVariantMap() },
        { "period", periodCandidate },
        { "selected", selectedCandidate },
        { "result", result },
        { "message", QString("%1\nscore threshold=%2, boundary=%3, zero ref=%4, one ref=%5\n%6\n%7\nnext sample: %8")
                .arg(phase)
                .arg(c_minScore, 0, 'f', 3)
                .arg(periodCandidate["classificationBoundary"].toInt())
                .arg(periodCandidate["zeroExpectedLength"].toInt())
                .arg(periodCandidate["oneExpectedLength"].toInt())
                .arg(candidateSummary("period", periodCandidate))
                .arg(result)
                .arg(static_cast<int>(m_experimentalDebugSample)) },
    };
    emit experimentalDebugChanged(m_experimentalDebugChannel);
    return !selectedCandidate.empty();
}

bool WaveformParser::inspectExperimentalDebugAt(uint chNum, int sample)
{
    if (sample < 0 || chNum >= mWavReader.getNumberOfChannels()) {
        return false;
    }

    const auto channel { chNum == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1() };
    if (channel.isNull() || channel->empty() || sample >= channel->size()) {
        setExperimentalDebugInactive(chNum, "Invalid debug sample.");
        return false;
    }

    const auto& parserSettings { ParserSettingsModel::instance()->getParserSettings() };
    const double sampleRate { static_cast<double>(mWavReader.getSampleRate()) };
    constexpr double c_minScore { 0.68 };
    QVariantMap periodCandidate { findExperimentalPeriodCandidate(*channel, static_cast<size_t>(sample), parserSettings, sampleRate) };
    QVariantMap selectedCandidate;
    const bool periodAccepted { periodCandidate["valid"].toBool() && periodCandidate["score"].toDouble() >= c_minScore };
    if (periodAccepted) {
        selectedCandidate = periodCandidate;
    }

    m_experimentalDebugChannel = chNum;
    m_experimentalDebugActive = true;
    m_experimentalDebugManualInspection = true;
    m_experimentalDebugSample = static_cast<size_t>(sample);
    m_experimentalDebugState = {
        { "active", true },
        { "chNum", static_cast<int>(chNum) },
        { "sample", sample },
        { "requestedSample", sample },
        { "phase", QString("Manual experimental inspection") },
        { "zero", periodCandidate["bit"].toInt() == 0 ? periodCandidate : QVariantMap() },
        { "one", periodCandidate["bit"].toInt() == 1 ? periodCandidate : QVariantMap() },
        { "period", periodCandidate },
        { "selected", selectedCandidate },
        { "result", periodAccepted ? QString("Selected %1").arg(periodCandidate["bit"].toInt()) : QString("No confident period") },
        { "message", QString("Manual experimental inspection\nrequested sample=%1, window offset=%2\nscore threshold=%3, boundary=%4, zero ref=%5, one ref=%6\n"
                              "period: %7 bit=%8 score=%9 raw=%10 penalty=%11 greedy=%12 zero-prefix=%13\n"
                              "found begin=%14 end=%15 len=%16, axis=%17, range=%18\n"
                              "upper=%19@%20, lower=%21@%22")
                .arg(sample)
                .arg(periodCandidate["begin"].toInt() - sample)
                .arg(c_minScore, 0, 'f', 3)
                .arg(periodCandidate["classificationBoundary"].toInt())
                .arg(periodCandidate["zeroExpectedLength"].toInt())
                .arg(periodCandidate["oneExpectedLength"].toInt())
                .arg(periodCandidate["valid"].toBool() ? QString("valid") : QString("invalid"))
                .arg(periodCandidate["bit"].toInt())
                .arg(periodCandidate["score"].toDouble(), 0, 'f', 3)
                .arg(periodCandidate["rawScore"].toDouble(), 0, 'f', 3)
                .arg(periodCandidate["totalPenalty"].toDouble(), 0, 'f', 3)
                .arg(periodCandidate["greedyOnePenalty"].toDouble(), 0, 'f', 3)
                .arg(periodCandidate["zeroPrefixScore"].toDouble(), 0, 'f', 3)
                .arg(periodCandidate["begin"].toInt())
                .arg(periodCandidate["end"].toInt())
                .arg(periodCandidate["length"].toInt())
                .arg(periodCandidate["axis"].toDouble(), 0, 'f', 1)
                .arg(periodCandidate["range"].toDouble(), 0, 'f', 1)
                .arg(periodCandidate["upperLevel"].toDouble(), 0, 'f', 1)
                .arg(periodCandidate["upperSample"].toInt())
                .arg(periodCandidate["lowerLevel"].toDouble(), 0, 'f', 1)
                .arg(periodCandidate["lowerSample"].toInt()) },
    };
    emit experimentalDebugChanged(chNum);
    return periodAccepted;
}

void WaveformParser::stopExperimentalDebug()
{
    setExperimentalDebugInactive(m_experimentalDebugChannel, "Experimental debug stopped.");
}

void WaveformParser::cancelParsing()
{
    if (!m_parsingActive || m_parsingCancellationRequested) {
        return;
    }

    m_parsingCancellationRequested = true;
    setParsingProgress(true, m_parsingProgress, QString("Canceling parser..."));
}

void WaveformParser::clearParsingCancellation()
{
    if (!m_parsingCancellationRequested) {
        return;
    }

    m_parsingCancellationRequested = false;
    emit parsingProgressChanged();
}

QVariantMap WaveformParser::experimentalDebugState(uint chNum) const
{
    if (!m_experimentalDebugActive || chNum != m_experimentalDebugChannel) {
        return {
            { "active", false },
            { "chNum", static_cast<int>(chNum) },
        };
    }

    return m_experimentalDebugState;
}

WaveformParser::SaveTapResult WaveformParser::saveTap(uint chNum, const QString& fileName)
{
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return { SaveTapResultCode::NoParsedData, {} };
    }

    QFile f(fileName.isEmpty() ? QString("tape_%1_%2.tap").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh-mm-ss.zzz")).arg(chNum ? "R" : "L") : fileName);
    if (f.exists() && !f.remove()) {
        qDebug() << "Cannot remove existing TAP file:" << f.fileName() << f.errorString();
        return { SaveTapResultCode::CannotRemoveExistingFile, f.errorString() };
    }

    if (!f.open(QIODevice::WriteOnly)) {
        qDebug() << "Cannot open TAP file for writing:" << f.fileName() << f.errorString();
        return { SaveTapResultCode::CannotOpenFile, f.errorString() };
    }

    auto parsedDataSPtr { parsedDataPtr->getParsedData() };
    auto& parsedData = *parsedDataSPtr.data();
    const auto selectedBlockSet { mSelectedBlocks.value(chNum) };

    for (auto i = 0; i < parsedData.size(); ++i) {
        if (!selectedBlockSet.empty() && !selectedBlockSet.contains(i)) {
            continue;
        }

        QByteArray b;
        const auto& data { parsedData.at(i)->data };
        const uint16_t size = data.size();
        b.append(reinterpret_cast<const char *>(&size), sizeof(size));
        b.append(reinterpret_cast<const char *>(data.data()), size);
        f.write(b);
    }

    f.close();
    return {};
}

QVector<uint8_t> WaveformParser::getParsedWaveform(uint chNum) const {
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return {};
    }
    return *parsedDataPtr->getParsedWaveform();// mParsedWaveform[chNum];
}

QPair<QVector<QSharedPointer<ParsedData::DataBlock>>, QVector<bool>> WaveformParser::getParsedData(uint chNum) const {
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return {};
    }

    const auto parsedData { *parsedDataPtr->getParsedData() };
    const auto selectedBlockSet { mSelectedBlocks.value(chNum) };
    QVector<bool> selectedBlocks;
    selectedBlocks.reserve(parsedData.size());
    for (auto i = 0; i < parsedData.size(); ++i) {
        selectedBlocks.append(selectedBlockSet.empty() || selectedBlockSet.contains(i));
    }

    return { parsedData, selectedBlocks };
}

bool WaveformParser::isBlockSelected(uint chNum, int blockNum) const
{
    return mSelectedBlocks.value(chNum).contains(blockNum);
}

bool WaveformParser::isBlockParseError(uint chNum, int blockNum) const
{
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr || blockNum < 0) {
        return false;
    }

    const auto parsedData { parsedDataPtr->getParsedData() };
    if (parsedData == nullptr || blockNum >= parsedData->size()) {
        return false;
    }

    return parsedData->at(blockNum)->state != ParsedDataModel::OK;
}

void WaveformParser::setBlockSelected(uint chNum, int blockNum, bool selected)
{
    if (blockNum < 0) {
        return;
    }

    auto& selectedBlocks { mSelectedBlocks[chNum] };
    if (selected) {
        selectedBlocks.insert(blockNum);
    } else {
        selectedBlocks.remove(blockNum);
    }

    emit blockSelectionChanged(chNum);
}

void WaveformParser::toggleBlockSelection(uint chNum, int blockNum)
{
    setBlockSelected(chNum, blockNum, !isBlockSelected(chNum, blockNum));
}

void WaveformParser::clearBlockSelection(uint chNum)
{
    mSelectedBlocks[chNum].clear();
    emit blockSelectionChanged(chNum);
}

QVariantList WaveformParser::selectedBlocks(uint chNum) const
{
    QVariantList result;
    const auto selectedBlocks { mSelectedBlocks.value(chNum) };
    for (auto block: selectedBlocks) {
        result.append(block);
    }
    std::sort(result.begin(), result.end(), [](const QVariant& left, const QVariant& right) {
        return left.toInt() < right.toInt();
    });
    return result;
}

int WaveformParser::getBlockDataStart(uint chNum, uint blockNum) const
{
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return 0;
    }
    auto parsedDataSPtr { parsedDataPtr->getParsedData() };
    auto& parsedData { *parsedDataSPtr };

    if (blockNum < (unsigned) parsedData.size()) {
        return parsedData[blockNum]->dataStart;
    }
    return 0;
}

int WaveformParser::getBlockDataEnd(uint chNum, uint blockNum) const
{
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return 0;
    }
    auto parsedDataSPtr { parsedDataPtr->getParsedData() };
    auto& parsedData { *parsedDataSPtr };

    if (blockNum < (unsigned) parsedData.size()) {
        return parsedData[blockNum]->dataEnd;
    }
    return 0;
}

int WaveformParser::getPositionByAddress(uint chNum, uint blockNum, uint addr) const
{
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return 0;
    }
    auto parsedDataSPtr { parsedDataPtr->getParsedData() };
    auto& parsedData { *parsedDataSPtr };

    if (blockNum < (unsigned) parsedData.size() && addr < (unsigned) parsedData[blockNum]->waveformData.size()) {
        return parsedData[blockNum]->waveformData[addr].begin;
    }
    return 0;
}

QPointer<ParsedDataModel> WaveformParser::getParsedChannelData(uint chNum) const
{
    auto* p = getParsedDataPtr(chNum);
    if (p == nullptr) {
        return { };
    }
    QQmlEngine::setObjectOwnership(p, QQmlEngine::CppOwnership);
    return p;

    //return { getParsedDataPtr(chNum) };

    // auto parsedDataPtr { getParsedDataPtr(chNum) };
    // if (parsedDataPtr == nullptr) {
    //     return { };
    // }

    // auto parsedDataSPtr { parsedDataPtr->getParsedData() };
    // auto& parsedData { *parsedDataSPtr };

    // static QMap<int, QString> blockTypes {
    //     {0x00, "Program"},
    //     {0x01, "Number Array"},
    //     {0x02, "Character Array"},
    //     {0x03, "Bytes"}
    // };
    // const QString id_header { qtTrId(ID_HEADER) };
    // const QString id_code { qtTrId(ID_CODE) };
    // const QString id_ok { qtTrId(ID_OK) };
    // const QString id_error { qtTrId(ID_ERROR) };
    // const QString id_unknown { qtTrId(ID_UNKNOWN) };

    // QVariantList r;
    // uint blockNumber = 0;

    // for (const auto& i: parsedData) {
    //     QVariantMap m;

    //     m.insert("block", QVariantMap { {"blockSelected", blockNumber < (unsigned) mSelectedBlocks.size() ? mSelectedBlocks[blockNumber] : (mSelectedBlocks.append(true), true)}, {"blockNumber", blockNumber++} });
    //     if (i.data.size() > 0) {
    //         auto d = i.data.at(0);
    //         int blockType = -1;
    //         auto btIt = blockTypes.end();
    //         QString blockTypeName;
    //         if (d == 0x00 && i.data.size() > 1) {
    //             d = i.data.at(1);
    //             btIt = blockTypes.find(d);
    //             blockType = btIt == blockTypes.end() ? -1 : d;
    //             blockTypeName = blockType == -1 ? QString::number(d, 16) : *btIt;
    //         }
    //         else {
    //             blockType = -2;
    //             blockTypeName = d == 0x00 ? id_header : id_code;
    //         }
    //         m.insert("blockType", blockTypeName);
    //         QString sizeText = QString::number(i.data.size());
    //         if (i.data.size() > 13 && btIt != blockTypes.end()) {
    //             sizeText += QString(" (%1)").arg(i.data.at(13) * 256 + i.data.at(12));
    //         }
    //         m.insert("blockSize", sizeText);
    //         QString nameText;
    //         if (blockType >= 0) {
    //             const auto loopRange { std::min(decltype(i.data.size())(12), i.data.size()) };
    //             nameText = QByteArray((const char*) &i.data.data()[2], loopRange > 1 ? loopRange - 2 : 0);
    //         }
    //         m.insert("blockName", nameText);
    //         m.insert("blockStatus", (i.state == ParsedData::OK ? id_ok : id_error) + qtTrId(ID_PARITY_MESSAGE).arg(QString::number(i.parityCalculated, 16).toUpper().rightJustified(2, '0')).arg(QString::number(i.parityAwaited, 16).toUpper().rightJustified(2, '0')));
    //         m.insert("state", i.state);
    //     }
    //     else {
    //         m.insert("blockType", id_unknown);
    //         m.insert("blockName", QString());
    //         m.insert("blockSize", 0);
    //         m.insert("blockStatus", id_unknown);
    //     }
    //     r.append(m);
    // }

    // return r;
}

ParsedDataModel* WaveformParser::getParsedChannel0() const
{
    return getParsedChannelData(0);
}

ParsedDataModel* WaveformParser::getParsedChannel1() const
{
    return getParsedChannelData(1);
}

bool WaveformParser::getParsingActive() const
{
    return m_parsingActive;
}

bool WaveformParser::getParsingCancellationRequested() const
{
    return m_parsingCancellationRequested;
}

int WaveformParser::getParsingProgress() const
{
    return m_parsingProgress;
}

QString WaveformParser::getParsingStatus() const
{
    return m_parsingStatus;
}

WaveformParser* WaveformParser::instance()
{
    static WaveformParser p;
    return &p;
}
