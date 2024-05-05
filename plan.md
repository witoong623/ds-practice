# Analytic base on type and configuration
- Define supported high level analysis.
    - Vehicle crossing line counting.
- Define configuration for each high level analysis.
    - Configuration must be per source. If this source doesn't want this analysis, don't do that.
- For each high level analysis
    - Everything must be per source.
    - Define data structure that hold necessary measurement tool (i.e. Line).
    - Define state of analysis. If it is vehicle crossing line counting, there must be count variable.