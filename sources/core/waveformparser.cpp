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
#include "sources/models/parsersettingsmodel.h"
#include <QPointer>
#include <QDebug>
#include <QDateTime>
#include <QByteArray>
#include <QVariantMap>
#include <QQmlEngine>
#include <algorithm>
#include <deque>

#define HARDCODED_DATA_SIGNAL_DELTA 0.75

WaveformParser::WaveformParser(QObject* parent) :
    QObject(parent),
    mWavReader(*WavReader::instance())
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

void WaveformParser::parse(uint chNum)
{
    if (chNum >= mWavReader.getNumberOfChannels()) {
        qDebug() << "Trying to parse channel that exceeds overall number of channels";
        return;
    }

    QWavVector& channel = *(chNum == 0 ? mWavReader.getChannel0() : mWavReader.getChannel1());
    QVector<ParsedData::WaveformPart> parsed = parseChannel<QWavVectorType>(channel);

    const double sampleRate = mWavReader.getSampleRate();
    auto& parsedData = *getOrCreateParsedDataPtr(chNum);
    parsedData.clear(channel.size());

    auto currentState = SEARCH_OF_PILOT_TONE;
    auto it = parsed.begin();
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

    parsedData.beginParse();

    while (currentState != NO_MORE_DATA) {
        auto prevIt = it;
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

    if (chNum == 0) {
        emit parsedChannel0Changed();
    }
    else {
        emit parsedChannel1Changed();
    }
}

void WaveformParser::saveTap(uint chNum, const QString& fileName)
{
    auto parsedDataPtr { getParsedDataPtr(chNum) };
    if (parsedDataPtr == nullptr) {
        return;
    }

    QFile f(fileName.isEmpty() ? QString("tape_%1_%2.tap").arg(QDateTime::currentDateTime().toString("dd.MM.yyyy hh-mm-ss.zzz")).arg(chNum ? "R" : "L") : fileName);
    f.remove(); //Remove file if exists
    f.open(QIODevice::WriteOnly);

    auto parsedDataSPtr { parsedDataPtr->getParsedData() };
    auto& parsedData = *parsedDataSPtr.data();
    for (auto i = 0; i < parsedData.size(); ++i) {
        if (i < mSelectedBlocks.size() && !mSelectedBlocks[i]) {
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
    return { *parsedDataPtr->getParsedData(), mSelectedBlocks };
}

void WaveformParser::toggleBlockSelection(int blockNum) {
    if (blockNum < mSelectedBlocks.size()) {
        auto& blk = mSelectedBlocks[blockNum];
        blk = !blk;

        emit parsedChannel0Changed();
        emit parsedChannel1Changed();
    }
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

WaveformParser* WaveformParser::instance()
{
    static WaveformParser p;
    return &p;
}
