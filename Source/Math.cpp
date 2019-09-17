#include "Math.h"

namespace Waveless
{
	static std::vector<unsigned int> FFTSize = { 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048 };

	double Math::DB2LinearMag(double dB)
	{
		return std::pow(10, dB / 10.0);
	}

	double Math::Linear2dBMag(double linear)
	{
		return 10.0 * std::log10(linear);
	}

	double Math::DB2LinearAmp(double dB)
	{
		return std::pow(10, dB / 20.0);
	}

	double Math::Linear2dBAmp(double linear)
	{
		return 20.0 * std::log10(linear);
	}

	ComplexArray Math::GenerateSine(double A, double f, double phi, double fs, double t)
	{
		auto N = size_t(std::round(t * fs));
		ComplexArray x(N);

		auto l_Intensity = DB2LinearAmp(A);

		for (size_t i = 0; i < N; i++)
		{
			x[i] = l_Intensity * std::cos(2 * PI * f * (double)i / fs + phi);
		}

		return x;
	}

	ComplexArray Math::DFT(const ComplexArray& x)
	{
		auto N = x.size();
		ComplexArray X(N);

		for (size_t k = 0; k < N; k++)
		{
			auto s = Complex(0.0, 0.0);

			for (size_t i = 0; i < N; i++)
			{
				s += x[i] * std::exp(Complex(0.0, -1.0) * 2.0 * PI * (double)k * ((double)i / (double)N));
			}

			X[k] = s;
		}

		return X;
	}

	ComplexArray Math::IDFT(const ComplexArray & X)
	{
		auto N = X.size();
		ComplexArray x(N);

		for (size_t k = 0; k < N; k++)
		{
			auto s = Complex(0.0, 0.0);

			for (size_t i = 0; i < N; i++)
			{
				s += (1.0 / (double)N) * X[i] * std::exp(Complex(0.0, 1.0) * 2.0 * PI * (double)k * ((double)i / (double)N));
			}

			x[k] = s;
		}

		return x;
	}

	std::vector<ComplexArray> Math::FFT(const ComplexArray & x)
	{
		// @TODO: window and zero padding
		auto N = x.size();
		bool l_isSizeSmallerThanFFTSize = false;

		for (auto i : FFTSize)
		{
			if (N <= i)
			{
				N = i;
				l_isSizeSmallerThanFFTSize = true;
				break;
			}
		}

		std::vector<ComplexArray> l_result;

		if (!l_isSizeSmallerThanFFTSize)
		{
			ComplexArray l_X(N);

			auto l_FFTChunkSize = FFTSize[FFTSize.size() - 1];
			auto l_lastChunkSize = N % l_FFTChunkSize;
			auto l_chunkCount = (N - l_lastChunkSize) / l_FFTChunkSize;

			// full size chunks
			for (size_t i = 0; i < l_chunkCount; i++)
			{
				auto l_chunk = ComplexArray(x[std::slice(i * l_FFTChunkSize, l_FFTChunkSize, 1)]);
				FFT_SingleFrame(l_chunk);
				l_result.emplace_back(l_chunk);
			};

			// last chunk with zero padding
			ComplexArray l_paddedLastChunk(l_FFTChunkSize);
			auto l_lastChunk = ComplexArray(x[std::slice(l_chunkCount * l_FFTChunkSize, l_lastChunkSize, 1)]);

			for (size_t i = 0; i < l_lastChunk.size(); i++)
			{
				l_paddedLastChunk[i] = l_lastChunk[i];
			}

			auto l_zeroPaddingSize = l_FFTChunkSize - l_lastChunkSize;
			for (size_t i = 0; i < l_zeroPaddingSize; i++)
			{
				l_paddedLastChunk[i + l_lastChunkSize] = Complex(0.0, 0.0);
			}

			FFT_SingleFrame(l_paddedLastChunk);
			l_result.emplace_back(l_paddedLastChunk);
		}
		else
		{
			// zero padding
			ComplexArray l_paddedChunk(N);

			for (size_t i = 0; i < x.size(); i++)
			{
				l_paddedChunk[i] = x[i];
			}

			auto l_zeroPaddingSize = N - x.size();
			for (size_t i = 0; i < l_zeroPaddingSize; i++)
			{
				l_paddedChunk[i + x.size()] = Complex(0.0, 0.0);
			}

			FFT_SingleFrame(l_paddedChunk);

			l_result.emplace_back(l_paddedChunk);
		}

		return l_result;
	}

	ComplexArray Math::IFFT(const std::vector<ComplexArray> & X)
	{
		std::vector<Complex> l_vector;
		l_vector.reserve(X.size() * X[0].size());

		for (auto Xi : X)
		{
			IFFT_SingleFrame(Xi);
			l_vector.insert(std::end(l_vector), std::begin(Xi), std::end(Xi));
		}

		ComplexArray l_result(l_vector.data(), l_vector.size());

		return l_result;
	}

	void Math::FFT_SingleFrame(ComplexArray & x)
	{
		const size_t N = x.size();
		if (N <= 1)
		{
			return;
		}

		// Divide
		ComplexArray even = x[std::slice(0, N / 2, 2)];
		ComplexArray odd = x[std::slice(1, N / 2, 2)];

		// Conquer
		FFT_SingleFrame(even);
		FFT_SingleFrame(odd);

		// Combine
		for (size_t k = 0; k < N / 2; ++k)
		{
			Complex t = std::polar(1.0, -2 * PI * k / N) * odd[k];
			x[k] = even[k] + t;
			x[k + N / 2] = even[k] - t;
		}
	}

	void Math::IFFT_SingleFrame(ComplexArray & X)
	{
		// Conjugate the complex numbers
		X = X.apply(std::conj);

		// forward FFT
		FFT_SingleFrame(X);

		// Conjugate the complex numbers again
		X = X.apply(std::conj);

		// Scale the numbers
		X /= (double)X.size();
	}

	std::vector<FreqBinData> Math::FreqDomainSeries2FreqBin(const std::vector<ComplexArray>& X, double fs)
	{
		std::vector<FreqBinData> l_result;

		for (auto& Xi : X)
		{
			auto l_XBinData = FreqDomainSeries2FreqBin_SingleFrame(Xi, fs);
			l_result.emplace_back(l_XBinData);
		}

		return l_result;
	}

	std::vector<ComplexArray> Math::FreqBin2FreqDomainSeries(const std::vector<FreqBinData>& XBinData)
	{
		std::vector<ComplexArray> l_result;

		for (auto& XBinDatai : XBinData)
		{
			auto l_X = FreqBin2FreqDomainSeries_SingleFrame(XBinDatai);
			l_result.emplace_back(l_X);
		}

		return l_result;
	}

	FreqBinData Math::FreqDomainSeries2FreqBin_SingleFrame(const ComplexArray& X, double sampleRate)
	{
		FreqBinData XBinData;
		auto N = X.size();

		// X[0] is the DC Offset
		XBinData.m_DCOffset = X[0];

		// X[i] == X[N - i], symmetry of Fourier Transform
		// Then only first half of the data in frequent domain is useful, other half are duplication
		auto l_binSize = N / 2;
		XBinData.m_FreqBinArray.reserve(l_binSize);

		for (size_t i = 1; i < l_binSize + 1; i++)
		{
			auto freq = sampleRate * (double)i / (double)N;
			//auto amp_real = linear2dBMag(std::abs(X[i].real()));
			//amp_real = X[i].real() > 0.0 ? amp_real : -amp_real;
			auto amp_real = X[i].real();
			//auto amp_imag = linear2dBMag(std::abs(X[i].imag()));
			//amp_imag = X[i].imag() > 0.0 ? amp_imag : -amp_imag;
			auto amp_imag = X[i].imag();
			FreqBin bin(freq, Complex(amp_real, amp_imag));
			XBinData.m_FreqBinArray.emplace_back(bin);
		}

		return XBinData;
	}

	ComplexArray Math::FreqBin2FreqDomainSeries_SingleFrame(const FreqBinData & XBinData)
	{
		auto N = XBinData.m_FreqBinArray.size();
		auto l_dataSize = N * 2;
		ComplexArray X(l_dataSize);

		// X[0] is the DC Offset
		X[0] = XBinData.m_DCOffset;

		// First half of data
		for (size_t i = 0; i < N; i++)
		{
			//auto amp_real = dB2LinearMag(XBinData[i].second.real());
			//amp_real = XBinData[i].second.real() > 0.0 ? amp_real : -amp_real;
			auto amp_real = XBinData.m_FreqBinArray[i].second.real();
			//auto amp_imag = dB2LinearMag(XBinData[i].second.imag());
			//amp_imag = XBinData[i].second.imag() > 0.0 ? amp_imag : -amp_imag;
			auto amp_imag = XBinData.m_FreqBinArray[i].second.imag();
			X[i + 1] = Complex(amp_real, amp_imag);
		}

		// Second half of data, without XBinData[N - 1]
		for (size_t i = 0; i < N; i++)
		{
			auto amp_real = X[N - i].real();
			auto amp_imag = X[N - i].imag();
			X[i + N] = Complex(amp_real, amp_imag);
		}

		return X;
	}

	ComplexArray Math::Synth(const std::vector<FreqBinData>& XBinData)
	{
		std::vector<Complex> l_vector;
		l_vector.reserve(XBinData.size() * XBinData[0].m_FreqBinArray.size());

		for (auto XBinDatai : XBinData)
		{
			auto l_x = Synth_SingleFrame(XBinDatai);
			l_vector.insert(std::end(l_vector), std::begin(l_x), std::end(l_x));
		}

		ComplexArray l_result(l_vector.data(), l_vector.size());

		return l_result;
	}

	ComplexArray Math::Synth_SingleFrame(const FreqBinData & XBinData)
	{
		auto l_X = FreqBin2FreqDomainSeries_SingleFrame(XBinData);
		IFFT_SingleFrame(l_X);
		return l_X;
	}
}