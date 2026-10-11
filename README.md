# Lexical Recode

| Version | **1.21.2 only** |

A recoded, open-source version of **Lexical**, focused on improved stability, cleaner code, modernized rendering, and modules better suited to the current state of **IgnPVP**.

This project aims to build upon the original Lexical codebase by refining existing systems, improving crash resistance, and modernizing the overall architecture while maintaining functionality relevant to the current Minecraft Bedrock PvP environment.

## Overview

Lexical Recode is a reworked version of Lexical that prioritizes stability, maintainability, and modernization. Existing systems have been cleaned up and restructured, with a focus on reducing instability and providing a more reliable foundation for continued development.

The project also introduces an updated rendering approach, combining **ImGui** and **Direct2D** rather than relying exclusively on Direct2D. This provides a more flexible foundation for rendering interfaces and visual elements throughout the client.

## Features

- **Open Source** — Publicly available source code for transparency, collaboration, and community-driven development.
- **Improved Stability** — Code refinements and structural changes intended to reduce crashes and improve overall reliability.
- **Modernized Codebase** — Cleaner, more organized code designed to make future maintenance and development easier.
- **Updated Modules** — Existing modules have been reworked to better suit the current state of IgnPVP.
- **Hybrid Rendering System** — Uses ImGui alongside Direct2D instead of relying solely on Direct2D.
- **Continued Development** — Provides a foundation for further improvements, optimizations, and new features.

## Rendering

Lexical Recode moves away from its original Direct2D-only rendering approach and adopts a hybrid rendering system built around:

- **ImGui** — For interface components and interactive UI elements.
- **Direct2D** — Misc.

This combination provides greater flexibility when developing and maintaining the client's interface and rendering systems.

**Important:** This project is specifically intended for Minecraft Bedrock Edition **1.21.2**. Other game versions are not officially supported and may require additional modifications.

## Project Goals

The primary goals of Lexical Recode are to:

- Improve stability and reduce the frequency of crashes.
- Clean up and modernize existing implementations.
- Adapt modules to the current IgnPVP environment.
- Improve the flexibility and maintainability of the rendering system.
- Establish a cleaner foundation for future development.

## Disclaimer

Lexical Recode is an independent, open-source recode of Lexical. Compatibility and stability may vary depending on the environment and configuration. Contributions, bug reports, and suggestions are welcome.
