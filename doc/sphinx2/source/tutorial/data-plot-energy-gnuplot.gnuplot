#!/usr/bin/env gnuplot
#
# created Tue Feb 10 11:15:39 2015 (20150210_11:15)
#
set term wxt 0 persist
set xlabel ' t=S.time'
set grid
set datafile missing 'nan'
set ylabel 'elast,grav,kinetic,nonviscDamp,$\sum$energy'
set y2label 'relative error'
set y2tics
plot  "< bzcat data-plot-energy-gnuplot.data.bz2" using 9:4 title '← elast(t)' with lines, "< bzcat data-plot-energy-gnuplot.data.bz2" using 9:5 title '← grav(t)' with lines, "< bzcat data-plot-energy-gnuplot.data.bz2" using 9:6 title '← kinetic(t)' with lines, "< bzcat data-plot-energy-gnuplot.data.bz2" using 9:7 title '← nonviscDamp(t)' with lines, "< bzcat data-plot-energy-gnuplot.data.bz2" using 9:1 title '← $\sum$energy(t)' with lines, "< bzcat data-plot-energy-gnuplot.data.bz2" using 9:8 title 'relative error(t) →' with lines axes x1y2
set term wxt 1 persist
set xlabel 't=S.time'
set grid
set datafile missing 'nan'
set ylabel '$z_1$,$z_0$'
plot  "< bzcat data-plot-energy-gnuplot.data.bz2" using 9:3 title '← $z_1$(t)' with lines, "< bzcat data-plot-energy-gnuplot.data.bz2" using 9:2 title '← $z_0$(t)' with lines
