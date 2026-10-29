
#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <array>
#include <chrono>
#include <filesystem>
#include <string>
#include <algorithm>
#include <cmath>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;
using Clock = chrono::high_resolution_clock;

const int THREAD_COUNT = 4;
const int BENCHMARK_RUNS = 30;

double elapsedMs(Clock::time_point start, Clock::time_point end)
{
    return chrono::duration<double, milli>(end - start).count();
}

// 1. 8-bit grayscale -> 6-bit quantization
Mat quantize6Bit(const Mat& gray)
{
    Mat result(gray.size(), CV_8UC1);

    for (int y = 0; y < gray.rows; ++y)
    {
        for (int x = 0; x < gray.cols; ++x)
        {
            uchar pixel = gray.at<uchar>(y, x);
            uchar sixBitValue = pixel >> 2; // 0-255 -> 0-63
            result.at<uchar>(y, x) = sixBitValue << 2;
        }
    }

    return result;
}

// Brightness adjustment for a rectangular region
void adjustRegion(Mat& image, Rect region, int offset)
{
    for (int y = region.y; y < region.y + region.height; ++y)
    {
        for (int x = region.x; x < region.x + region.width; ++x)
        {
            int value = image.at<uchar>(y, x) + offset;
            image.at<uchar>(y, x) =
                saturate_cast<uchar>(value);
        }
    }
}

void adjustFourRegions(Mat& image)
{
    int halfW = image.cols / 2;
    int halfH = image.rows / 2;

    vector<Rect> regions = {
        Rect(0, 0, halfW, halfH),
        Rect(halfW, 0, image.cols - halfW, halfH),
        Rect(0, halfH, halfW, image.rows - halfH),
        Rect(halfW, halfH, image.cols - halfW, image.rows - halfH)
    };

    int offsets[4] = { -60, -20, 20, 60 };
    vector<thread> workers;

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        workers.emplace_back([&, i]()
            {
                adjustRegion(image, regions[i], offsets[i]);
            });
    }

    for (auto& worker : workers)
        worker.join();
}

void adjustSingleThread(Mat& image)
{
    int halfW = image.cols / 2;
    int halfH = image.rows / 2;

    vector<Rect> regions = {
        Rect(0, 0, halfW, halfH),
        Rect(halfW, 0, image.cols - halfW, halfH),
        Rect(0, halfH, halfW, image.rows - halfH),
        Rect(halfW, halfH, image.cols - halfW, image.rows - halfH)
    };

    int offsets[4] = { -60, -20, 20, 60 };

    for (int i = 0; i < THREAD_COUNT; ++i)
        adjustRegion(image, regions[i], offsets[i]);
}

// 4. Histogram calculation
array<int, 256> histogramRegion(const Mat& image, Rect region)
{
    array<int, 256> histogram{};
    histogram.fill(0);

    for (int y = region.y; y < region.y + region.height; ++y)
    {
        for (int x = region.x; x < region.x + region.width; ++x)
            histogram[image.at<uchar>(y, x)]++;
    }

    return histogram;
}

array<int, 256> histogramSingleThread(const Mat& image)
{
    return histogramRegion(image, Rect(0, 0, image.cols, image.rows));
}

array<int, 256> histogramFourThreads(const Mat& image)
{
    int halfW = image.cols / 2;
    int halfH = image.rows / 2;

    vector<Rect> regions = {
        Rect(0, 0, halfW, halfH),
        Rect(halfW, 0, image.cols - halfW, halfH),
        Rect(0, halfH, halfW, image.rows - halfH),
        Rect(halfW, halfH, image.cols - halfW, image.rows - halfH)
    };

    array<int, 256> localHistograms[4];
    vector<thread> workers;

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        workers.emplace_back([&, i]()
            {
                localHistograms[i] = histogramRegion(image, regions[i]);
            });
    }

    for (auto& worker : workers)
        worker.join();

    array<int, 256> combined{};
    combined.fill(0);

    for (int i = 0; i < THREAD_COUNT; ++i)
        for (int bin = 0; bin < 256; ++bin)
            combined[bin] += localHistograms[i][bin];

    return combined;
}

Mat drawHistogram(const array<int, 256>& histogram)
{
    const int width = 512;
    const int height = 300;

    Mat canvas(height, width, CV_8UC3, Scalar(255, 255, 255));

    int maxValue = *max_element(histogram.begin(), histogram.end());
    if (maxValue == 0)
        return canvas;

    for (int i = 0; i < 256; ++i)
    {
        int x1 = i * 2;
        int x2 = x1 + 1;

        int barHeight = static_cast<int>(
            static_cast<double>(histogram[i]) / maxValue * (height - 30));

        line(canvas,
            Point(x1, height - 1),
            Point(x1, height - 1 - barHeight),
            Scalar(0, 0, 0), 1);

        line(canvas,
            Point(x2, height - 1),
            Point(x2, height - 1 - barHeight),
            Scalar(0, 0, 0), 1);
    }

    return canvas;
}

int main()
{
    try
    {
        string inputPath = "city.jpg";
        string outputDir = "results";
        fs::create_directories(outputDir);

        Mat color = imread(inputPath, IMREAD_COLOR);

        if (color.empty())
        {
            cerr << "HATA: city.jpg bulunamadi.\n";
            cerr << "Dosyayi proje klasorune koy: " << fs::current_path() << endl;
            return 1;
        }

        Mat gray;
        cvtColor(color, gray, COLOR_BGR2GRAY);

        cout << "Goruntu boyutu: " << gray.cols
            << " x " << gray.rows << "\n\n";

        // 1. 8-bit to 6-bit quantization
        Mat quantized = quantize6Bit(gray);
        imwrite(outputDir + "/01_grayscale.png", gray);
        imwrite(outputDir + "/02_quantized_6bit.png", quantized);

        cout << "1) 8-bit gri goruntu 6-bit seviyelerine kuantize edildi.\n";

        // 2 and 3. Brightness: single thread vs four threads
        Mat singleResult;
        Mat multiResult;

        auto startSingle = Clock::now();
        for (int i = 0; i < BENCHMARK_RUNS; ++i)
        {
            singleResult = gray.clone();
            adjustSingleThread(singleResult);
        }
        auto endSingle = Clock::now();

        auto startMulti = Clock::now();
        for (int i = 0; i < BENCHMARK_RUNS; ++i)
        {
            multiResult = gray.clone();
            adjustFourRegions(multiResult);
        }
        auto endMulti = Clock::now();

        double singleBrightnessMs = elapsedMs(startSingle, endSingle) / BENCHMARK_RUNS;
        double multiBrightnessMs = elapsedMs(startMulti, endMulti) / BENCHMARK_RUNS;

        imwrite(outputDir + "/03_brightness_four_regions.png", multiResult);

        cout << "\n2-3) Parlaklik ayarlama (ortalama " << BENCHMARK_RUNS << " calistirma):\n";
        cout << "Tek thread: " << singleBrightnessMs << " ms\n";
        cout << "4 thread:   " << multiBrightnessMs << " ms\n";

        if (multiBrightnessMs > 0)
            cout << "Hizlanma orani: " << singleBrightnessMs / multiBrightnessMs << "x\n";

        cout << "Tek ve coklu thread sonuclari ayni mi? "
            << (countNonZero(singleResult != multiResult) == 0 ? "Evet" : "Hayir")
            << "\n";

        // 4 and 5. Histogram: single thread vs four threads
        array<int, 256> serialHistogram;
        array<int, 256> parallelHistogram;

        auto startHistSingle = Clock::now();
        for (int i = 0; i < BENCHMARK_RUNS; ++i)
            serialHistogram = histogramSingleThread(gray);
        auto endHistSingle = Clock::now();

        auto startHistMulti = Clock::now();
        for (int i = 0; i < BENCHMARK_RUNS; ++i)
            parallelHistogram = histogramFourThreads(gray);
        auto endHistMulti = Clock::now();

        double singleHistMs = elapsedMs(startHistSingle, endHistSingle) / BENCHMARK_RUNS;
        double multiHistMs = elapsedMs(startHistMulti, endHistMulti) / BENCHMARK_RUNS;

        bool histogramsEqual = (serialHistogram == parallelHistogram);

        imwrite(outputDir + "/04_histogram.png", drawHistogram(serialHistogram));

        cout << "\n4-5) Histogram hesaplama (ortalama " << BENCHMARK_RUNS << " calistirma):\n";
        cout << "Tek thread: " << singleHistMs << " ms\n";
        cout << "4 thread:   " << multiHistMs << " ms\n";
        cout << "Histogramlar ayni mi? " << (histogramsEqual ? "Evet" : "Hayir") << "\n";

        if (multiHistMs > 0)
            cout << "Hizlanma orani: " << singleHistMs / multiHistMs << "x\n";

        // 6. 0.75 * pixel vs 0.75 * pixel + 20
        Mat multipliedOnly;
        Mat multipliedPlus20;

        gray.convertTo(multipliedOnly, CV_8UC1, 0.75, 0.0);
        gray.convertTo(multipliedPlus20, CV_8UC1, 0.75, 20.0);

        imwrite(outputDir + "/05_multiply_075.png", multipliedOnly);
        imwrite(outputDir + "/06_multiply_075_plus_20.png", multipliedPlus20);

        Mat difference;
        absdiff(multipliedOnly, multipliedPlus20, difference);

        Scalar meanDifference = mean(difference);
        int changedPixels = countNonZero(difference);

        cout << "\n6) Piksel islemleri:\n";
        cout << "A: pixel * 0.75\n";
        cout << "B: pixel * 0.75 + 20\n";
        cout << "Ortalama mutlak piksel farki: " << meanDifference[0] << "\n";
        cout << "Farki olan piksel sayisi: " << changedPixels
            << " / " << gray.total() << "\n";

        imwrite(outputDir + "/07_difference.png", difference);

        cout << "\nTum ciktilar '" << outputDir << "' klasorune kaydedildi.\n";
        cout << "Program basariyla tamamlandi.\n";

        return 0;
    }
    catch (const exception& e)
    {
        cerr << "HATA: " << e.what() << endl;
        return 1;
    }
}
