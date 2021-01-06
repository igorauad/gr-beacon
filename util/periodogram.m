function [ Pk ] = periodogram( x, Nfft )
%PERIODOGRAM Compute the periodogram of signal x
%   Compute the spectral density estimate as |X[k]|^2 / Nfft, i.e., the
%   absolute squared of the FFT divided by the FFT length.

assert(mod(length(x), Nfft) == 0)

n_blocks = length(x) / Nfft;

x_r = reshape(x, Nfft, n_blocks);

mean_mag_sq = mean(abs(fft(x_r, Nfft)).^2, 2);

Pk = mean_mag_sq / Nfft;

end
