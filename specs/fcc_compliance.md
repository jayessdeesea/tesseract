# FCC Part 15 Compliance

## Regulatory Framework
- **Applicable rule:** FCC Part 15.209 (intentional radiator below 490 kHz)
- **Field strength limit:** 2400/F(kHz) μV/m measured at 300 meters
- **At 60 kHz:** 2400/60 = **40 μV/m at 300 meters**
- **Measurement method:** Calibrated loop antenna, quasi-peak detector

## Compliance Strategy

### Conservative Design Approach
1. **Ferrite rod antenna is inherently inefficient** at 60 kHz
   - Wavelength = 5 km; antenna is electrically tiny (~0.00004λ)
   - Radiation efficiency is extremely low by physics
2. **Building attenuation** contains signal within structure
3. **Low drive current** (starting at ~50 mA through coil)
4. **Reference:** Similar Instructables designs achieve 30+ foot range, well under limits

### Why No Formal Measurement
- Calibrated field strength meter costs ~$10k+
- Conservative design stays well below limits by orders of magnitude
- Building structure contains signal
- 60 kHz is not in any active commercial/safety band

### Risk Mitigation
- Start with lowest drive current, increase only if needed
- Monitor for RFI complaints (extremely unlikely at 60 kHz)
- If enforcement action occurs: reduce power or cease operation
- No duty cycle restriction under Part 15 (continuous operation legal)

## Estimated Field Strength Budget
```
Transmit power:     ~50 mW (worst case)
Antenna efficiency: ~0.001% (electrically tiny ferrite rod)
Effective radiated:  ~0.5 μW
At 300m:            << 1 μV/m (well below 40 μV/m limit)
```

## Interference Considerations
- 60 kHz is in the VLF band, below AM broadcast (530 kHz)
- No consumer electronics operate at 60 kHz
- Nearest concern: AM radio (530+ kHz), harmonics would need to reach there
- 2nd harmonic (120 kHz): still below AM, still under Part 15 limits
- Ferrite rod has naturally narrow bandwidth (low harmonic radiation)
