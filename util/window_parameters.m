function [ cg, enbw, sl ] = window_parameters( w )
%WINDOW_PARAMETERS Find relevant windowing figures of merit
%   Compute the coherent gain (CG), equivalent noise bandwidth (ENBW), and
%   the scallop loss (SL).

Nfft = length(w);

% coherent gain
cg = sum(w) / Nfft;

% equivalent noise bw
enbw = Nfft * norm(w)^2 / sum(w)^2;

% Scallop loss in dB
w_freq = fft(w, 2*Nfft);
sl = 20*log10(abs(w_freq(2)) / sum(w));

end
