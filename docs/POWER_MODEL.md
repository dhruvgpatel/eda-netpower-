# netpower: Dynamic Power Model & Methodology

## Mathematical Model

The dynamic power dissipation of a CMOS logic network is modeled as:

$$P_{\text{dynamic}} = 0.5 \times \alpha \times C \times V^2 \times f$$

Where:
- $\alpha$: Switching activity factor (dimensionless).
- $C$: Net capacitance in Farads (F).
- $V$: Supply voltage in Volts (V).
- $f$: Operating switching frequency in Hertz (Hz).
- $P_{\text{dynamic}}$: Estimated power in Watts (W), reported in nanowatts (nW, $1 \text{ nW} = 10^{-9} \text{ W}$).

### Alpha Convention
In `netpower`, $\alpha_{\text{net}}$ is defined as the **total number of state transitions ($0 \to 1$ plus $1 \to 0$) per net per measured vector interval**:

$$\alpha_{\text{net}} = \frac{T_{\text{rising}} + T_{\text{falling}}}{N - 1}$$

Where:
- $N$ is the total number of applied input vectors ($N \ge 2$).
- $N - 1$ is the number of transitions between consecutive vectors.
- $T_{\text{rising}}$ is the count of low-to-high transitions ($0 \to 1$).
- $T_{\text{falling}}$ is the count of high-to-low transitions ($1 \to 0$).

Under this definition, the factor $0.5$ in the power equation represents the average energy transferred per state transition ($\frac{1}{2} C V^2$), because charging the capacitance from $0 \to V$ draws $C V^2$ from the power supply, while discharging from $V \to 0$ draws no energy from the supply.

---

## Capacitance Modeling

- **Default Capacitance**: Specified via `--default-capacitance-ff <val>` in femtofarads ($1 \text{ fF} = 10^{-15} \text{ F}$).
- **CSV Overrides**: An optional CSV file supplied via `--capacitance-file <file.csv>` allows individual nets to have custom capacitance values:
```csv
net,capacitance_fF
sum,4.5
carry,3.2
```

---

## Limitations and Assumptions

1. **Zero-Delay Simulation**: Logic evaluations occur instantaneously in topological order without inertial or transport gate delays. Consequently, glitches and hazard-induced spurious transitions are not captured.
2. **Dynamic Switched Capacitance Only**: The model estimates switched load power $P = \frac{1}{2} \alpha C V^2 f$. It excludes:
   - Static leakage power ($I_{\text{leak}} \times V$).
   - Internal cell switching power (charging/discharging of internal diffusion/gate nodes).
   - Short-circuit (crowbar) power during finite input slew.
3. **No Characterized Library Tables**: Commercial sign-off tools (e.g. PrimeTime PX) rely on non-linear delay/power tables in Liberty (`.lib`) models indexed by input transition time and output capacitive load. `netpower` is an educational tool and does not read `.lib` files.
4. **Input Vector Independence**: Primary inputs are modeled as uncorrelated Bernoulli(0.5) random variables. Real workloads exhibit spatial and temporal correlation.

