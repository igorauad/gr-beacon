% Useful reference:
% https://kluedo.ub.uni-kl.de/frontdoor/deliver/index/docId/4293/file/exact_fft_measurements.pdf
clearvars, clc

%% Global parameters
fs = 240e3;
Nfft = 512;
n_samples = 1000*Nfft;
snr_db = 20;
snr_lin = 10^(snr_db/10);
ampl = 1.0;
Ftarget = 0.1;  % target digital frequency (generate nearby frequencies)

%% Derived parameters
delta_f = fs / Nfft;  % FFT frequency spacing
fft_bin = 1 / Nfft;  % FFT digital frequency spacing (or bin width)
n = 0:(n_samples-1);  % time axis
F = (0:(Nfft-1)) * fft_bin;  % digital frequency axis

%% Complex wave
% Generate wave frequencies aligned and misaligned with FFT bins. The
% misaligned frequency is halfway between two FFT bins.
F_aligned = round(Ftarget * Nfft) / Nfft;  % round to an FFT bin
F_misaligned = F_aligned + (fft_bin / 2);  % halfway between two FFT bins

% Corresponding signals
s_aligned = ampl * exp(j * 2 * pi * F_aligned * n);
s_misaligned = ampl * exp(j * 2 * pi * F_misaligned * n);

%% Noise
% Generate noise to achieve a target SNR.

s_power = ampl^2;  % A^2 for complex wave; A^2/2 for real sine-wave
noise_power = s_power / snr_lin;
noise = sqrt(Nfft * noise_power/2) * ...
    complex(randn(1, n_samples), randn(1, n_samples));

%% Window design

%w = window(@rectwin, Nfft);
%w = window(@blackmanharris, Nfft);
%w = window(@blackman, Nfft);
%w = window(@rectwin, Nfft);
%w = window(@hann, Nfft);
w = window(@flattopwin, Nfft); % better to avoid scalloping loss
[cg, enbw, sl] = window_parameters(w);

cpg = 20*log10(cg);  % coherent power gain
enbw_db = 10*log10(enbw);  % ENBW in dB

%% Windowing coherent gain
% The coherent gain effect refers to the DC gain of the window, which is
% not unitary and, therefore, scales the FFT results. It can be more easily
% observed on a complex exponential that is aligned with an FFT bin, i.e.,
% a wave that is not subject to scalloping loss.

% Observe the power spectral density before and after windowing. The peak
% density should differ according to the window's coherent power gain.
Pk_rect = periodogram(s_aligned, Nfft);
Pk_windowed = periodogram_win(s_aligned, w, Nfft);
Pk_rect_db = 10*log10(Pk_rect);
Pk_windowed_db = 10*log10(Pk_windowed);

figure
plot(F, Pk_rect_db)
hold on
plot(F, Pk_windowed_db)
legend('Non-windowed', 'Windowed')
xlabel('Digital frequency')
ylabel('Magnitude (dB)')
grid on
title("Coherent gain window effect")

fprintf("--\nCoherent power gain effect:\n\n")
fprintf(1, "Window's coherent power gain: %f dB\n", cpg);
fprintf(1, "Observed difference on peak power: %f dB\n", ...
    max(Pk_rect_db) - max(Pk_windowed_db));

%% Scalloping loss
% The scalloping loss effect is observed when the sine/complex wave
% frequency is not aligned with an FFT bin. In this case, the PSD magnitude
% will not be determined by the DC gain of the window. Instead, it will be
% another (lower) point in the window's frequency response, depending on
% the frequency offset relative to the FFT bin.
%
% This effect can be observed by comparing the peak power corresponding to
% the aligned and misaligned complex exponentials. In this case, we can use
% the windowed versions of both signals while comparing to the nominal
% scalloping loss available on variable "sl".
Pk_aligned = periodogram_win(s_aligned, w, Nfft);
Pk_misaligned = periodogram_win(s_misaligned, w, Nfft);
Pk_aligned_db = 10*log10(Pk_aligned);
Pk_misaligned_db = 10*log10(Pk_misaligned);

figure
plot(F, Pk_aligned_db)
hold on
plot(F, Pk_misaligned_db)
legend('Aligned freq.', 'Misaligned freq.')
xlabel('Digital frequency')
ylabel('Magnitude (dB)')
grid on
title("Scalloping loss effect")

observed_gain_diff = max(Pk_aligned_db) - max(Pk_misaligned_db);

fprintf(1, "--\nScalloping loss effect:\n\n")
fprintf(1, "Window's scalloping loss: %f dB\n", sl);
fprintf(1, "Observed difference on peak power: %f dB\n", observed_gain_diff);

%% Equivalent noise bandwidth (ENBW) effect
% The ENBW indicates how much more white/broadband noise the adopted window
% "receives" compared to the rectangular window. A non rectangular window
% leads to an increased displayed noise floor.
%
% This effect can be demonstrated by comparing the power spectral density
% of a white Gaussian noise sequence, i.e., the noise floor. After
% windowing, we should expect a different noise floor. Nevertheless, note
% that the noise florr difference will come from two windowing effects: 1)
% the ENBW effect; 2) the coherent power gain (CPG). Both ENBW and CPG are
% unitary (or 0 dB) for the rectangular window, so the difference in noise
% floor with a non-rectangular window is simply the sum of the ENBW and CPG
% corresponding to the adopted window.
%
% When measuring the CNR, however, note that the CPG affects both the
% carrier and the noise, so it doesn't alter the CNR. In contrast, the ENBW
% alters the noise floor only, so it does affect the CNR measurement.
Pk_rect = periodogram(noise, Nfft);
Pk_windowed = periodogram_win(noise, w, Nfft);
Pk_rect_db = 10*log10(Pk_rect);
Pk_windowed_db = 10*log10(Pk_windowed);

figure
plot(F, Pk_rect_db)
hold on
plot(F, Pk_windowed_db)
legend('Non-windowed', 'Windowed')
xlabel('Digital frequency')
ylabel('Magnitude (dB)')
grid on
title("ENBW effect")

fprintf(1, "--\nENBW effect:\n\n")
fprintf(1, "Window's ENBW + CPG: %f dB\n", enbw_db + cpg);
fprintf(1, "Observed noise floor difference: %f dB\n", ...
    mean(Pk_rect_db) - mean(Pk_windowed_db));

%% SNR Estimation for complex sine-wave under AWGN
% Consider two scenarios:
% 1) Complex sine-wave at integer multiple of the FFT spacing;
% 2) Complex sine-wave at non-integer multiple of the FFT spacing.

signal = {s_aligned, s_misaligned};
label = {"Aligned frequency", "Misaligned frequency"};

for i = 1:length(signal)
    % Noisy complex wave
    s_n = signal{i} + noise;

    % Periodogram estimate
    Pk_rect = periodogram(s_n, Nfft);
    Pk_windowed = periodogram_win(s_n, w, Nfft);

    figure
    plot(F, 10*log10(Pk_rect))
    hold on
    plot(F, 10*log10(Pk_windowed))
    legend('Non-windowed', 'Windowed')
    xlabel('Digital frequency')
    ylabel('Magnitude (dB)')
    grid on
    title(sprintf("%s (non-windowed)", label{i}))

    fprintf(1, "--\nCNR estimation:\n\n")

    % Possible CNR computations

    % 1) Based on the non-windowed spectrogram
    cnr_db_0 = 10*log10(cnr(Pk_rect, Nfft));
    fprintf(1, "%s (non-windowed) CNR: %f dB\n", label{i}, cnr_db_0);

    % 2) Based on the windowed spectrogram
    cnr_db_1 = 10*log10(cnr(Pk_windowed, Nfft));
    fprintf(1, "%s (windowed) CNR: %f dB\n", label{i}, cnr_db_1);

    % 3) Based on the windowed spectrogram, corrected for the ENBW effect
    cnr_db_2 = cnr_db_1 + 10*log10(enbw);
    fprintf(1, "%s (windowed + ENBW) CNR: %f dB\n", label{i}, cnr_db_2);

    % 4) Based on the windowed spectrogram, corrected for the ENBW effect
    % and the scalloping loss effect. That should be helpful only for the
    % misaligned frequency, but it should deteriorate the estimate for the
    % aligned frequency. In other words, it's not a general purpose
    % approach.
    cnr_db_3 = cnr_db_1 - sl + 10*log10(enbw);
    fprintf(1, "%s (windowed + ENBW + SL) CNR: %f dB\n", label{i}, cnr_db_3);
end

% Conclusions: approach #3 leads to better results in general. When the
% frequency is known, the scalloping loss can be compensated as in #4. In
% the specific case of a sine-wave or complex wave under AWGN, when the
% wave frequency is unknown, it is hard to compensate for the scalloping
% loss. In this case, it's better to use a window with low scalloping loss,
% like the flat-top window.
