function [ cnr ] = cnr( Pk, Nfft )
%CNR Estimate the CNR of a CW beacon under AWGN
%   Based on the periodogram vector Pk with Nfft points, find the beacon CW
%   peak power density and estimate the noise floor. Then, estimate the
%   corresponding CNR.

[peak_mag, i_max] = max(Pk);

n_neigh = 8;
i_s = mod((i_max - n_neigh), Nfft);
i_e = mod((i_max + n_neigh), Nfft);

if (i_s > i_e)
    floor_samples = Pk(i_e:i_s);
else
    floor_samples = vertcat(Pk(i_e:end), Pk(1:i_s));
end

n_floor_samples = Nfft - (2*n_neigh - 1);
assert(length(floor_samples) == n_floor_samples);

floor_mag = sum(floor_samples) / n_floor_samples;

% C/N = (C+N)/N -1
cnr = (peak_mag / floor_mag) - 1;

end
