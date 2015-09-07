---
layout: default
title: Parallel Graph Algorithms on the Xeon Phi Coprocessor
---

# Parallel Graph Algorithms on the Xeon Phi Coprocessor
August 2015

## Master Thesis of Dennis Felsing
Karlsruhe Institute of Technology  
At the Department of Informatics  
Institute of Theoretical Computer Science  
Parallel Computing Group

Reviewer: Juniorprof. Dr. Henning Meyerhenke  
Advisor: Moritz von Looz

## Abstract
Complex networks have received interest in a wide area of applications, ranging from road networks over hyperlink connections in the world wide web to interactions between people.
Advanced algorithms are required for the generation as well as visualization of such graphs.

In this work two graph algorithms, one for graph generation, the other for graph visualization, are studied exemplarily.
We detail the work of adapting and porting the algorithms to the Intel Xeon Phi coprocessor architecture.
Problems in porting real software projects and used libraries are encountered and solved.
Memory allocations turned out to be a major problem for the graph generation algorithm.
The limited memory of the Xeon Phi forced us to offload chunks of the data from the host system to the Xeon Phi, which impeded performance, eliminating any significant speedup.

The data sets consisting of at most 365000 edges for the graph visualization algorithm fit into the Xeon Phi's memory easily, which simplified the porting process significantly.
We achieve a speedup for sparse graphs over the host system containing two 8-core Intel Xeon (Sandy Bridge) processors.
While the hot inner loop by itself can utilize the 512-bit vector instructions of the Xeon Phi, the benefit disappears when embedded in the more complicated full program.

## Material
- [Thesis](thesis.pdf) ([BibTeX](thesis.bib), [source](thesis.tar.xz))
- [Slides](beamer.pdf) ([source](beamer.tar.xz))
