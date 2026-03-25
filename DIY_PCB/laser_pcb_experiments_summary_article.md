# Home Laser PCB Prototyping: Experiments, Findings, and Practical Settings

## Why I started doing this

I started these experiments because PCB iteration through JLCPCB was too slow for the way I work. A typical order could take around three weeks to arrive, and on top of that I sometimes made mistakes in the design. That meant every error cost a lot of time. The feedback loop was too slow.

So I wanted a way to make PCBs at home.

There were two main directions:

1. CNC isolation milling
2. Laser-based masking / ablation followed by chemical etching

I chose the laser path because it seemed cheaper, smaller, and potentially more precise for the kind of small prototype boards I wanted to make. I bought a Mecpow M1 laser, around 800 lei, and began by engraving wood just to understand the machine, LaserGRBL, and the general workflow.

At first I tried exporting from EasyEDA through image-like approaches such as PNG, and also vector-like exports such as SVG or DXF. Those attempts produced incomplete or poor results, so I kept looking for a better workflow.

Then I explored the actual PCB tooling side. There are also multiple sub-approaches here:

- paint or coating removal with laser, followed by etching
- dry film / photoresist workflows using transparencies and UV
- CNC-style isolation toolpaths repurposed for laser motion

I tried CopperCAM first. The free version was confusing because some contours were not being generated, and the reason turned out to be the limit of 25 contours in the free version.

FlatCAM ended up being a better fit. The interface was still not trivial, and installation was annoying enough that I created a virtual machine with Ubuntu 22.04 just to run it reliably. Still, FlatCAM had advantages:

- easier export flow once understood
- better compatibility with LaserGRBL
- useful bounding box output, which helps place the job correctly on the PCB blank

My first deliberately simple success was a tiny resistor + LED board with a few wire pads. That worked well enough to prove that the method could work.

After that I attempted a more serious board using an ATtiny816, two resistors, one capacitor, and one LED. The QFN format was painful: painful to engrave, painful to etch cleanly, and painful to solder. I failed around twenty times before getting a usable result. That became the real motivation for running controlled experiments instead of continuing with intuition and luck.

---

## Goal of the experiments

The purpose of these experiments was to understand the practical limits and tradeoffs of my home laser PCB process:

- how many contours to use
- how much overlap to use between contours
- what speed to choose
- what laser power to choose
- how many passes to run
- what trace widths and clearances are realistically manufacturable

I was not only trying to find the absolute limit. I was trying to find the **sweet spot** between:

- fine enough geometry for real compact PCBs
- short enough engraving time
- sufficient repeatability
- reasonable solderability
- not over-thinning the copper features

In other words: not “what can barely be done once,” but “what can be done reliably enough to be useful.”

---

## Test structure

In EasyEDA I created a test PCB containing traces of different widths, starting around 0.2 mm and going upward through larger values such as 0.3 mm and beyond, up to around 1 mm. The exact width ladder was not the main point of this phase; the more important variables were the laser process settings.

Then in FlatCAM I generated multiple versions of the job with different combinations of:

- contour count
- overlap
- speed
- power

I used naming in this style:

- `1c_120s_100p`
- `2c_0.1o_120s_100p`
- `3c_0.5o_120s_100p`
- `12c_0.1o_600s_1000p`

Where:

- `c` = number of contours
- `o` = overlap
- `s` = speed
- `p` = power

And specifically:

- speed values were `120` or `600`
- power values were `100` or `1000`
- overlap values were `0.1` or `0.5`, meaning 10% or 50%

The tested combinations included:

1. `1c_120s_100p`
2. `2c_0.1o_120s_100p`
3. `2c_0.5o_120s_100p`
4. `3c_0.1o_120s_100p`
5. `3c_0.5o_120s_100p`
6. `12c_0.1o_120s_100p`
7. `1c_120s_1000p`
8. `1c_600s_1000p`
9. `2c_0.5o_600s_1000p`
10. `12c_0.1o_600s_1000p`

For some of these I also compared 1 pass, 2 passes, and 3 passes.

---

## Visual ranking data

These rankings were based on quick visual inspection of the PCBs rather than microscope-level analysis, so they should be treated as provisional but still useful.

### 1 pass ranking

Best to worst:

- 7
- 3
- 8
- 9
- 5
- 4
- 1
- 2
- 6
- 10

Mapped to settings:

- **7** = `1c_120s_1000p`
- **3** = `2c_0.5o_120s_100p`
- **8** = `1c_600s_1000p`
- **9** = `2c_0.5o_600s_1000p`
- **5** = `3c_0.5o_120s_100p`
- **4** = `3c_0.1o_120s_100p`
- **1** = `1c_120s_100p`
- **2** = `2c_0.1o_120s_100p`
- **6** = `12c_0.1o_120s_100p`
- **10** = `12c_0.1o_600s_1000p`

### 2 passes ranking

Best to worst:

- 8
- 7
- 5
- 9
- 2
- 1
- 6
- 10

Mapped to settings:

- **8** = `1c_600s_1000p`
- **7** = `1c_120s_1000p`
- **5** = `3c_0.5o_120s_100p`
- **9** = `2c_0.5o_600s_1000p`
- **2** = `2c_0.1o_120s_100p`
- **1** = `1c_120s_100p`
- **6** = `12c_0.1o_120s_100p`
- **10** = `12c_0.1o_600s_1000p`

### 3 passes ranking

Best to worst:

- 7
- 5
- 9
- 4
- 1
- 2
- 6
- 10

Mapped to settings:

- **7** = `1c_120s_1000p`
- **5** = `3c_0.5o_120s_100p`
- **9** = `2c_0.5o_600s_1000p`
- **4** = `3c_0.1o_120s_100p`
- **1** = `1c_120s_100p`
- **2** = `2c_0.1o_120s_100p`
- **6** = `12c_0.1o_120s_100p`
- **10** = `12c_0.1o_600s_1000p`

### What the rankings change

The rankings add an important nuance.

At a purely visual level, several **high-power single-contour settings** rank surprisingly well, especially:

- `1c_120s_1000p`
- `1c_600s_1000p`
- `2c_0.5o_600s_1000p`

This means that if the only criterion is how cleanly exposed the channels look to the eye, high-power ablation can appear very successful.

However, this does **not** automatically overturn the earlier caution about dimensional accuracy. A setting can rank highly visually while still narrowing traces or damaging edge fidelity. So the rankings strengthen the case that there may be **two different winners** depending on the goal:

1. **visual clearing winner**
2. **geometry-preserving winner**

That distinction is central to interpreting the experiments.

## Observations

### 1. Number of passes

A recurring observation was that more passes could help remove leftover paint, but the relationship was not always linear.

For example:

- In `1c_120s_100p`, 1 pass sometimes looked cleaner than expected.
- In `2c_0.1o_120s_100p`, 2 passes still seemed to leave some paint, while 3 passes looked better.
- In `12c_0.1o_600s_1000p`, 3 passes also looked better than 2, and 2 better than 1.

So the practical trend appears to be:

- **1 pass can work when the coating is favorable**
- **2 passes often improves consistency**
- **3 passes can further improve clearing, but also raises risk of collateral heating**

This suggests that pass count is acting as a compensation tool for variations in:

- paint thickness
- focus consistency
- surface flatness
- coating absorption

A very important nuance is that a “visually cleaner” line is not automatically a better PCB feature. More passes may remove coating more completely, but they can also narrow the final copper trace or distort the edge.

### 2. Effect of contour count

Going from 1 contour to 2 or 3 contours generally improved the effective cleared channel, especially when paint remained between lines.

However, there is a diminishing return:

- 1 contour is fast, but often too narrow or incomplete
- 2 contours is a meaningful upgrade
- 3 contours seems like a practical upper-middle ground
- 12 contours becomes excessive for most normal boards unless used for a very specific purpose

A high contour count mainly helps if the line spacing between contours is correct. If the overlap is too low, many contours still leave visible unburned material between passes.

### 3. Overlap: 10% versus 50%

This was one of the clearer findings.

The 50% overlap setting looked better than 10% overlap.

With 10% overlap, the laser tracks were still separated enough that black paint remained between contours. Even if the gaps were only a few pixels wide in the image, those remaining strips matter a lot during etching.

With 50% overlap, the remaining material was reduced significantly. Even when some residue was still visible, it often looked lighter or grayish instead of clearly intact black paint.

Interpretation:

- **10% overlap is too conservative for this process**
- **50% overlap is much closer to what the beam geometry actually needs**

That makes sense physically: if the effective burned line is not perfectly uniform, a larger overlap prevents the process from relying on a perfectly calibrated beam width.

### 4. Speed: 120 versus 600

At high speed, the lines started looking more jittery or wobbly, especially on horizontal or curved features. There was also more evidence of incomplete paint removal.

This suggests two separate effects:

1. lower energy deposited per unit length
2. greater visibility of mechanical motion artifacts / acceleration effects

So although 600 improves throughput dramatically, it appears to reduce feature quality.

Still, speed is not useless. For less demanding work, a faster speed combined with extra passes may be a very reasonable compromise.

### 5. Power: 100 versus 1000

High power made the copper much more visible, which initially looks attractive because it suggests better paint removal.

But the downside was important:

- surrounding paint seemed to melt or change structure
- edges looked thermally affected
- the actual copper feature appeared to become thinner

For a 0.2 mm trace, the effective result could look closer to 0.1 mm after processing. That is a serious problem for fine-pitch footprints such as QFN, where even a small amount of edge loss can make alignment and soldering much harder.

So high power seems to create a false sense of success:

- visually cleaner copper exposure
- but worse dimensional accuracy

This is a crucial tradeoff. If the goal is just to expose copper, high power can appear successful. If the goal is to preserve geometry, high power can be destructive.

### 6. “Melted” paint around the track

On some settings the paint around the track looked melted or structurally changed rather than cleanly removed. That likely means the coating entered a thermally modified state rather than being sharply ablated.

That could affect etching in multiple ways:

- it might still protect copper partially
- it might lift or flake irregularly
- it might leave a softened edge that changes final trace shape

This is another reason why “most exposed copper” is not the same thing as “best process.” The best process is the one that gives sharp, dimensionally faithful edges with repeatability.

---

## Synthesis: what the results seem to say

With the visual rankings included, the picture becomes more precise.

### Main process trends

1. **50% overlap still appears better than 10% overlap**.
   - Setting 5 (`3c_0.5o_120s_100p`) consistently ranks above setting 4 (`3c_0.1o_120s_100p`) when both appear.
   - Setting 3 (`2c_0.5o_120s_100p`) outranks setting 2 (`2c_0.1o_120s_100p`) in the 1-pass ranking.

2. **12 contours looks consistently poor**.
   - Settings 6 and 10 remain near the bottom of the rankings.
   - This strongly suggests that very high contour counts are not a productive default for this process.

3. **High power ranks visually very well**.
   - Settings 7 and 8 rank at or near the top.
   - Setting 9 also performs strongly.
   - This confirms that aggressive ablation exposes copper effectively to the eye.

4. **The likely geometry-preserving winner is different from the visual winner**.
   - Visually, settings 7 and 8 are top contenders.
   - But based on your own notes, those same settings also risk melting surrounding paint and thinning traces.
   - Therefore, they are probably best described as **visual clearing winners**, not necessarily **manufacturing winners** for fine geometry.

5. **Setting 5 is the strongest balanced candidate**.
   - `3c_0.5o_120s_100p` ranks well, especially for 2 and 3 passes.
   - It also aligns with the qualitative observations favoring higher overlap, lower speed, and gentler power.
   - This makes it the best candidate for the default "serious PCB" setting.

### Two separate “winners” now emerge

#### Winner A: visual-clearing winner

The strongest visual-clearing candidates are:

- `1c_120s_1000p` (setting 7)
- `1c_600s_1000p` (setting 8)
- `2c_0.5o_600s_1000p` (setting 9)

These settings seem best if the goal is:

- fast visible copper exposure
- rough boards
- generous geometries
- minimal concern about edge fidelity

#### Winner B: balanced manufacturing winner

The strongest balanced candidate is:

- `3c_0.5o_120s_100p` (setting 5)

Why this one stands out:

- ranks strongly in 2-pass and 3-pass use
- uses the better overlap strategy
- avoids the thermal aggression of full power
- preserves the lower-speed motion quality
- aligns better with the need to keep traces and pads dimensionally trustworthy

So if the question is not “what looks best immediately after engraving?” but rather “what should I trust for real compact boards?”, then setting 5 is the current best default.

### Revised practical interpretation

The rankings suggest this:

- for **rough/simple boards**, the high-power settings may genuinely be useful
- for **small package boards**, the safer answer is still a lower-power, higher-overlap, multi-contour setting

This means the optimum is not one universal preset. It is project-dependent.

## Provisional recommended settings

These are not final universal rules, but they are the most defensible generic settings based on your notes.

### A. Fine-detail / small-package setting

Use when the board contains small pads, narrow clearances, or fine-pitch ICs such as:

- SOIC/SOP if routed tightly
- TSSOP
- small QFN attempts

Suggested starting point:

- **Contours:** 3
- **Overlap:** 50%
- **Speed:** 350
- **Power:** around 100 to 500
- **Passes:** 2

If paint removal is incomplete:

- keep geometry the same
- try **3 passes before drastically increasing power**

Why this is the likely winner:

- it stays close to your best observations
- it avoids the thermal edge damage seen at 1000 power
- it uses the better overlap
- it keeps the cleaner, more stable motion quality of the lower speed

### B. General prototype setting

Use when the board is moderate density and you care about time as well as acceptable quality.

Typical targets:

- 0603 / 0805 passives
- SOT-23
- SOIC
- through-hole plus some SMD

Suggested starting point:

- **Contours:** 2
- **Overlap:** 50%
- **Speed:** somewhere between 120 and 250, ideally an intermediate value if available
- **Power:** around 150 to 250
- **Passes:** 2

This setting is likely to save time while still staying within the better-behaved region of the parameter space.

### C. Fast rough board setting

Use when trace widths and clearances are generous, for example:

- LED boards
- power wiring
- simple breakout boards
- 2.54 mm pitch parts

Suggested starting point:

- **Contours:** 2
- **Overlap:** 50%
- **Speed:** 600
- **Power:** moderate-high, but not necessarily full
- **Passes:** 2

This setting accepts some wobble and dimensional imprecision in exchange for time.

### D. Settings to avoid as default

Avoid using these as your generic baseline:

- **1 contour** for anything non-trivial
- **10% overlap** for serious PCB work
- **1000 power** for fine geometry unless a specific coating demands it
- **12 contours** except as a special-purpose stress test

---

## How close the likely winner is to my best-ranked settings

With the ranking data included, the answer becomes more explicit.

### If “winner” means visually best after engraving

Then the winners are closest to:

- **setting 7** = `1c_120s_1000p`
- **setting 8** = `1c_600s_1000p`
- **setting 9** = `2c_0.5o_600s_1000p`

These dominate the rankings more than expected.

### If “winner” means best compromise for actual PCB making

Then the winner is closest to:

- **setting 5** = `3c_0.5o_120s_100p`

This is the strongest balanced preset because it performs well visually while also matching the non-visual concerns I identified:

- better overlap
- less thermal damage
- less obvious trace shrinkage
- cleaner geometry at lower speed

## Suggested process targets by package type

### Through-hole / 2.54 mm pitch / easy boards

Examples:

- DIP
- headers
- large connectors
- simple LED boards

Recommended process bias:

- prioritize speed
- slight line wobble acceptable
- dimensional shrinkage not very critical

Suggested laser preset:

- 2 contours
- 50% overlap
- higher speed acceptable
- moderate power
- 2 passes

### SOIC / SOP / SOT-23 / 0603–0805 boards

Examples:

- common analog/digital boards
- small microcontroller support circuits
- practical prototype boards

Recommended process bias:

- preserve edge fidelity
- avoid over-thinning traces
- keep etch channels reliably clean

Suggested laser preset:

- 2 to 3 contours
- 50% overlap
- low speed or moderate speed
- low-to-moderate power
- 2 passes, possibly 3 when coating is inconsistent

This is likely your main sweet spot.

### TSSOP / denser compact routing

Examples:

- medium-pitch MCU boards
- denser sensor boards

Recommended process bias:

- quality over speed
- geometry preservation critical

Suggested laser preset:

- 3 contours
- 50% overlap
- low speed
- low power, increased only cautiously
- 2 or 3 passes

### QFN / fine-pitch work

Examples:

- ATtiny816 QFN attempts
- compact modern ICs

Recommended process bias:

- maximum dimensional fidelity
- minimal thermal spread
- accept slow processing

Suggested laser preset:

- 3 contours
- 50% overlap
- low speed
- as little power as still fully clears the coating
- 2 passes first, then 3 only if necessary

This should be treated as an experimental zone, not yet a production-ready home workflow.

---

## Important methodological insight

One of the key lessons from these experiments is that the visually most dramatic result is not necessarily the best result.

In this process, “better” should be defined as:

- sharp edge
- correct final trace width
- correct final pad shape
- reliable paint removal in channels
- minimal collateral thermal effect
- repeatability across the board

That means future ranking should emphasize **dimensional fidelity** and **etch success**, not only whether bare copper looks bright and obvious after engraving.

---
