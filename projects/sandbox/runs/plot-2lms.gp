
nr = `wc -l < two-lms.dat`
max = `cut -f1 two-lms.dat | tail -1`


do for [t=1:nr-1] {

   # set terminal postscript
   # outfile = sprintf('| ps2pdf - output%03.0f.pdf',t)

   # set terminal postscript eps size 3.5,2.62 enhanced color font 'Helvetica,20' linewidth 2
   set terminal postscript eps enhanced color linewidth 1
   # set terminal postscript eps
   outfile = sprintf('| epstopdf --filter > output%03.0f.pdf',t)

   set output outfile

   ###########################################
   ## plot BLEU scores
   ###########################################

   set title "BLEU score"
   # set xlabel "modification steps"
   # set key left top
   set key inside right bottom

   set logscale x 2
   unset xlabel
   unset ylabel

   set xrange [1:max]
   set yrange [0:0.3]
   set format x "2^{%L}"
   plot "two-lms.dat" every ::::t using 1:2 title "+ reverse LM" with linespoints,\
   	"default.dat" every ::::t using 1:2 title "default" with linespoints, \
	0.2653 title "Moses"

   unset multiplot

}
