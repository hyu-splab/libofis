# # <<Figure 6-1>>

# reset
# set title "BA-200" offset -15,-2.6 font ",20"
# set size 1,0.42
# set bmargin 0.3
# set xrange [*:*] noreverse writeback
# set yrange [0:35]
# set ylabel offset 1,0
# set ylabel "Exe. time (sec)"
# unset xtics
# set ytics 5,5,35
# set grid y
# set style data histograms
# set style histogram rowstacked gap 0
# set boxwidth 0.85
# set style fill solid border -1
# set terminal postscript eps enhanced color 18
# set output "graphs/6_1_exe-time-200.eps"
# plot newhistogram, 'SpMV/figures/ofis256-rank-200.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle, \
# newhistogram, 'SpMV/figures/es256-200.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle, \
# newhistogram, 'SpMV/figures/ew256-200.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle


# # <<Figure 6-2>>

# reset
# set title "BA-400" offset -15,-2.6 font ",20"
# set size 1,0.42
# set bmargin 0.3
# set xrange [*:*] noreverse writeback
# set yrange [0:60]
# set ylabel offset 1,0
# set ylabel "Exe. time (sec)"
# unset xtics
# set ytics 10,10,70
# set grid y
# set style data histograms
# set style histogram rowstacked gap 0
# set boxwidth 0.85
# set style fill solid border -1
# set terminal postscript eps enhanced color 18
# set output "graphs/6_2_exe-time-400.eps"
# plot newhistogram, 'SpMV/figures/ofis256-rank-400.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle, \
# newhistogram, 'SpMV/figures/es256-400.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle, \
# newhistogram, 'SpMV/figures/ew256-400.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle


# # <<Figure 6-3>>

# reset
# set title "BA-600" offset -15,-2.6 font ",20"
# set size 1,0.42
# set bmargin 0.3
# set lmargin 7.2
# set xrange [*:*] noreverse writeback
# set yrange [0:90]
# set ylabel offset 1,0
# set ylabel "Exe. time (sec)"
# unset xtics
# set ytics 20,20,80
# set grid y
# set style data histograms
# set style histogram rowstacked gap 0
# set boxwidth 0.85
# set style fill solid border -1
# set terminal postscript eps enhanced color 18
# set output "graphs/6_3_exe-time-600.eps"
# plot newhistogram, 'SpMV/figures/ofis256-rank-600.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle, \
# newhistogram, 'SpMV/figures/es256-600.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle, \
# newhistogram, 'SpMV/figures/ew256-600.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle


# # <<Figure 6-4>>

# reset
# set title "BA-800" offset -15,-2.6 font ",20"
# set size 1,0.5
# set lmargin 7.2 
# set bmargin screen 0.09
# set xrange [*:*] noreverse writeback
# set yrange [0:120]
# set ylabel offset 2,0
# set ylabel "Exe. time (sec)"
# set xtics offset 0,0.2 norotate nomirror font ",16"
# set ytics 0,20,120
# set grid y
# set key at screen 0.94,0.46 vertical invert samplen 3 spacing 1.1
# set style data histograms
# set style histogram rowstacked gap 0
# set boxwidth 0.85
# set style fill solid border -1
# set terminal postscript eps enhanced color 18
# set output "graphs/6_4_exe-time-800.eps"
# plot newhistogram "SpMV-OFIS" offset 0,-0.3, 'SpMV/figures/ofis256-rank-800.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle, \
# newhistogram "SparseP-ES" offset 0,-0.3, 'SpMV/figures/es256-800.txt' u 2:xtic(1) lc rgb "#0000ff" t "Input transfer", \
# '' u 3 lc rgb "#ffaaaa" t "DPU execution", \
# '' u 4 lc rgb "#ffffff" t "Output transfer", \
# newhistogram "SparseP-EW" offset 0,-0.3, 'SpMV/figures/ew256-800.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle


# # <<Figure 7>>

# reset
# set title "256x256 tiles" offset 0,-2.3
# set size 1,0.5
# set lmargin 6.5
# set rmargin 1.5
# set xrange [0:1100] 
# set yrange [0:6]
# set ylabel offset 1,0
# set ylabel "Speedup"
# set xlabel "No. of DPUs" offset 0,0.5
# set ytics
# set xtics (64,128,256,512,1024) nomirror
# set grid y
# set key bottom horizontal 
# set terminal postscript eps enhanced color 22
# set output "graphs/7_speedup-ofis-es256.eps"
# plot "SpMV/figures/speedup-ofis256-rank-es256.txt" u 1:2 w lp lw 3 ps 1.5 t "BA-100",\
# "" u 1:3 w lp lw 3 ps 1.5 t "BA-200",\
# "" u 1:4 w lp lw 3 ps 1.5 t "BA-400",\
# "" u 1:5 w lp lw 3 ps 1.5 t "BA-600",\
# "" u 1:6 w lp lw 3 ps 1.5 t "BA-800",\
# "" u 1:7 w lp lw 3 ps 1.5 t "BA-1000"


# # << Figure 10-1 >>

# reset
# set title "BA-100" offset 0,-2.5
# set size 1,0.42
# set bmargin 0.3
# set lmargin 6.5
# set rmargin 1.5
# set xrange [*:*] noreverse writeback
# set yrange [0:20]
# set ylabel "Exe. time (sec)"
# set ytics 5,5,25 
# unset xtics
# set grid y
# set style data histograms
# set style histogram rowstacked
# set boxwidth 0.85
# set style fill solid border -1
# set terminal postscript eps enhanced color 20
# set output "graphs/10_1_ofis-comp-100.eps"
# plot newhistogram, 'SpMV/figures/ofis256-rank-100.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle, \
# newhistogram, 'SpMV/figures/ofis512-rank-100.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle


# # << Figure 10-2 >>

# reset
# set title "BA-1000" offset 0,-2.5
# set size 1,0.5
# set bmargin screen 0.09
# set lmargin 6.5
# set rmargin 1.5
# set xrange [*:*] noreverse writeback
# set yrange [0:140]
# set ylabel offset 1,0
# set ylabel "Exe. time (sec)"
# set xtics offset 0,0.2 norotate nomirror font ",18"
# set ytics 0,20,140
# set grid y
# set key at screen 0.97,0.46 vertical invert samplen 3 spacing 1.1
# set style data histograms
# set style histogram rowstacked
# set boxwidth 0.85
# set style fill solid border -1
# set terminal postscript eps enhanced color 20
# set output "graphs/10_2_ofis-comp-1000.eps"
# plot newhistogram "256x256 tiles" offset 0,-0.3, 'SpMV/figures/ofis256-rank-1000.txt' u 2:xtic(1) lc rgb "#0000ff" t "Input transfer", \
# '' u 3 lc rgb "#ffaaaa" t "DPU execution", \
# '' u 4 lc rgb "#ffffff" t "Output transfer", \
# newhistogram "512x512 tiles" offset 0,-0.3, 'SpMV/figures/ofis512-rank-1000.txt' u 2:xtic(1) lc rgb "#0000ff" notitle, \
# '' u 3 lc rgb "#ffaaaa" notitle, \
# '' u 4 lc rgb "#ffffff" notitle


# # <<Figure 11>>

# reset
# set title "512x512 tiles" offset 0,-2.3
# set size 1,0.5
# set lmargin 6.5
# set rmargin 1.5
# set xrange [0:1100]
# set yrange [0:8]
# set ylabel offset 1,0
# set ylabel "Speedup"
# set xlabel "No. of DPUs" offset 0,0.5
# set ytics 2
# set xtics (64,128,256,512,1024) nomirror
# set grid y
# set key bottom horizontal
# set terminal postscript eps enhanced color 22
# set output "graphs/11_speedup-ofis-es512.eps"
# plot "SpMV/figures/speedup-ofis512-rank-es512.txt" u 1:2 w lp lw 3 ps 1.5 t "BA-100",\
# "" u 1:3 w lp lw 3 ps 1.5 t "BA-200",\
# "" u 1:4 w lp lw 3 ps 1.5 t "BA-400",\
# "" u 1:5 w lp lw 3 ps 1.5 t "BA-600",\
# "" u 1:6 w lp lw 3 ps 1.5 t "BA-800",\
# "" u 1:7 w lp lw 3 ps 1.5 t "BA-1000"


# # <<Figure 12>>

# reset
# set title "IG-unit vs rank-unit (512x512 tiles)" offset 0,-2.3
# set size 1,0.5
# set lmargin 6.5
# set rmargin 1.5
# set xrange [0:1100]
# set yrange [0.8:2.2]
# set ylabel "Speedup" offset 2,0
# set xlabel "No. of DPUs" offset 0,0.5
# set ytics 1.0,0.2,2.0
# set xtics (64,128,256,512,1024) nomirror
# set grid y
# set key bottom horizontal
# set terminal postscript eps enhanced color 22
# set output "graphs/12_speedup-ig-rank.eps"
# plot "SpMV/figures/speedup-ofis-rank-chip.txt" u 1:2 w lp lw 3 ps 1.5 t "BA-100",\
# "" u 1:3 w lp lw 3 ps 1.5 t "BA-200",\
# "" u 1:4 w lp lw 3 ps 1.5 t "BA-400",\
# "" u 1:5 w lp lw 3 ps 1.5 t "BA-600",\
# "" u 1:6 w lp lw 3 ps 1.5 t "BA-800",\
# "" u 1:7 w lp lw 3 ps 1.5 t "BA-1000"


# # <<Figure 13-1>>

# reset
# set title "SpMV-OFIS vs SparsP-ES with 512x512 tiles" offset 0,-2.3
# set size 1,0.38
# set lmargin 6.5
# set rmargin 1.5
# set bmargin 0.1
# set xrange [0:1100]
# set yrange [0:12]
# set ylabel "Speedup"
# set ytics (2,4,6,8,10)
# unset xtics 
# set grid y
# set key bottom horizontal
# set terminal postscript eps enhanced color 22
# set output "graphs/13_1_speedup-ofis-ig-es512.eps"
# plot "SpMV/figures/speedup-ofis512-ig-es512.txt" u 1:2 w lp lw 3 ps 1.5 notitle,\
# "" u 1:3 w lp lw 3 ps 1.5 notitle,\
# "" u 1:4 w lp lw 3 ps 1.5 notitle,\
# "" u 1:5 w lp lw 3 ps 1.5 notitle,\
# "" u 1:6 w lp lw 3 ps 1.5 notitle,\
# "" u 1:7 w lp lw 3 ps 1.5 notitle 


# # <<Figure 13-2>>

# reset
# set title "SpMV-OFIS (512x512 tiles) vs SparsP-EW (256x256 tiles)" offset 0,-2.3
# set size 1,0.5
# set lmargin 6.5
# set rmargin 1.5
# set xrange [0:1100]
# set yrange [1:4.5]
# set ylabel offset 2,0
# set ylabel "Speedup"
# set xlabel "No. of DPUs" offset 0,0.5
# set ytics (1.5,2.5,3.5)
# set xtics (64,128,256,512,1024) nomirror
# set grid y
# set key bottom horizontal
# set terminal postscript eps enhanced color 22
# set output "graphs/13_2_speedup-ofis-ig-ew256.eps"
# plot "SpMV/figures/speedup-ofis512-ig-ew256.txt" u 1:2 w lp lw 3 ps 1.5 t "BA-100",\
# "" u 1:3 w lp lw 3 ps 1.5 t "BA-200",\
# "" u 1:4 w lp lw 3 ps 1.5 t "BA-400",\
# "" u 1:5 w lp lw 3 ps 1.5 t "BA-600",\
# "" u 1:6 w lp lw 3 ps 1.5 t "BA-800",\
# "" u 1:7 w lp lw 3 ps 1.5 t "BA-1000"


# # <<Figure 8-1>>

# reset
# set title "PR-OFIS (DPU-unit) vs PR-WRAM with 1024 DPUs" offset 0,-2.3
# set size 1,0.4
# set lmargin 6.5
# set rmargin 1.5
# set bmargin 0.1
# set xrange [3900:10500]
# set yrange [0.9:1.6]
# set ylabel offset 2,0
# set ylabel "Speedup"
# set ytics (1.0, 1.2, 1.4)
# unset xtics 
# set grid y
# set key bottom horizontal
# set terminal postscript eps enhanced color 22
# set output "graphs/8_1_speedup-pr-dpu.eps"
# plot "PageRank/figures/speedup-pr-dpu.txt" u 1:2 w lp lw 3 ps 1.5 notitle,\
# "" u 1:3 w lp lw 3 ps 1.5 notitle,\
# "" u 1:4 w lp lw 3 ps 1.5 notitle,\
# "" u 1:5 w lp lw 3 ps 1.5 notitle,\
# "" u 1:6 w lp lw 3 ps 1.5 notitle,\


# # <<Figure 8-2>>

# reset
# set title "PR-OFIS (IG-unit) vs PR-WRAM with 1024 DPUs" offset 0,-2.3
# set size 1,0.52
# set lmargin 6.5
# set rmargin 1.5
# set xrange [3900:10500]
# set yrange [0.9:1.6]
# set ylabel offset 2,0
# set ylabel "Speedup"
# set xlabel "No. of Parts" offset 0,0.5
# set ytics (1.0,1.2,1.4)
# set xtics (4096,5120,6144,7168,8192,9216,10240) nomirror
# set grid y
# set key bottom horizontal
# set terminal postscript eps enhanced color 22
# set output "graphs/8_2_speedup-pr-ig.eps"
# plot "PageRank/figures/speedup-pr-ig.txt" u 1:2 w lp lw 3 ps 1.5 t "333SP",\
# "" u 1:3 w lp lw 3 ps 1.5 t "AS365",\
# "" u 1:4 w lp lw 3 ps 1.5 t "M6",\
# "" u 1:5 w lp lw 3 ps 1.5 t "NLR",\
# "" u 1:6 w lp lw 3 ps 1.5 t "rgg\\_n"

# # <<Figure 9>>
# reset
# set title "Regex-OFIS vs Regex-PIM" offset 0,-2.3
# set size 1,0.5
# set lmargin 6.5
# set rmargin 1.5
# set xrange [0:1100]
# set yrange [2:6]
# set ylabel offset 1,0
# set ylabel "Speedup"
# set xlabel "No. of DPUs" offset 0,0.5
# # set ytics (3,4,5,6)
# set ytics 2,1,6
# set xtics (64,128,256,512,1024) nomirror
# set grid y
# set key bottom horizontal
# set terminal postscript eps enhanced color 22
# set output "graphs/9_regex_speedup.eps"
# plot "Regex/figures/speedup.txt" u 1:2 w lp lw 3 ps 1.5 t "IG-unit",\
# "" u 1:3 w lp lw 3 ps 1.5 t "Rank-unit"

# << Figure 14-1 >>
reset
set size 0.5, 0.5
set bmargin screen 0.07
set lmargin 6.5
set rmargin 1.5
set xrange [0:*] noreverse writeback
set yrange [0:43]
set ylabel offset 2,0
set ylabel "Time (sec)" offset 1,0
set xtics nomirror font ",18"
set grid y
set key at graph 1, 0.99 right top vertical samplen 1 spacing 1.2 font ",17" invert
set style data histograms
set style histogram rowstacked gap 10
set boxwidth 0.85
set style fill solid border -1
set terminal postscript eps enhanced color 20
set output "graphs/14_1_e2e-breakdown_spmv.eps"
plot 'SpMV/figures/combined-600-e2e.txt' u 2:xtic(1) lc rgb "#8DD3C7" t "Computation", \
'' u 3 lc rgb "#aa8ee3" t "Partitioning"

# << Figure 14-2 >>
reset
set size 0.5, 0.5
set bmargin screen 0.07
set lmargin 6.5
set rmargin 1.5
set xrange [0:*] noreverse writeback
set yrange [0:190]
set ylabel offset 2,0
set ylabel "Time (sec)" offset 1,0
set xtics nomirror font ",18"
set ytics 30
set grid y
set key at graph 0, 0.99 left top horizontal samplen 1 spacing 1.2 font ",17"
set style data histograms
set style histogram rowstacked gap 10
set boxwidth 0.85
set style fill solid border -1
set terminal postscript eps enhanced color 20
set output "graphs/e2e-breakdown_regex.eps"
plot 'Regex/figures/combined.txt' u 2:xtic(1) lc rgb "#8DD3C7" t "Computation"