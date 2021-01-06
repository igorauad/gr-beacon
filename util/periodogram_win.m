function [ Pk ] = periodogram_win( x, w, Nfft )
%PERIODOGRAM_WIN Compute the windowed periodogram of signal x
%   Compute the spectral density estimate as |Xw[k]|^2 / Nfft, i.e., the
%   absolute squared of the FFT Xw[k] corresponding to the windowed signal,
%   divided by the FFT length.

assert(mod(length(x), Nfft) == 0)

n_blocks = length(x) / Nfft;

x_r = reshape(x, Nfft, n_blocks);

w_r = repmat(w, 1, n_blocks);

mean_mag_sq = mean(abs(fft(x_r .* w_r, Nfft)).^2, 2);

Pk = mean_mag_sq / Nfft;

end
