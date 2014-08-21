
nr = `wc -l < initial.dat`
max = `cut -f1 initial.dat | tail -1`

do for [t=1:nr-1] {

   # set terminal postscript
   # outfile = sprintf('| ps2pdf - output%03.0f.pdf',t)

   # set terminal postscript eps size 3.5,2.62 enhanced color font 'Helvetica,20' linewidth 2
   set terminal postscript eps enhanced color linewidth 1
   # set terminal postscript eps
   outfile = sprintf('| epstopdf --filter > output%03.0f.pdf',t)

   set output outfile

   # set object 1 rectangle from graph 0,0 to graph 1,1 behind fc rgb "#eeeeff"

   # set multiplot layout 2,1
   set multiplot layout 2,2
   # set multiplot layout 2,3

   # set xtic rotate by -30
   set logscale x 2
   # set logscale x 16


   set style data linespoints

   ###########################################
   ## plot sentence length initial
   ###########################################

   set title "sentence initial char"
   set ylabel "sentences with identical char"
   set ytics ("0" 0,"25" 10,"50" 20,"75" 30,"100" 40)

   # set xlabel "modification steps"
   unset xlabel
   # set size 0.5,0.5
   # set size 1,0.8
   set xrange [1:max]
   set yrange [0:40]
   ## xtic formats: see http://gnuplot.sourceforge.net/docs_4.2/node184.html
   set format x "2^{%L}"
   plot "initial.dat" every ::::t using 1:6 notitle with linespoints, \
	"initial.dat" every ::::t using 1:8 notitle with linespoints, \
	"initial.dat" every ::::t using 1:10 notitle with linespoints, \
	"initial.dat" every ::::t using 1:12 notitle with linespoints, \
	"initial.dat" every ::::t using 1:14 notitle with linespoints


   ###########################################
   ## plot BLEU scores
   ###########################################

   set title "BLEU score"
   # set xlabel "modification steps"
   # set key left top
   set key inside right bottom

   unset xlabel
   unset ylabel
   set ytics auto

   # set size 0.5,0.5
   # set size 1,0.8
   set xrange [1:max]
   set yrange [0:0.3]
   set format x "2^{%L}"
   plot "initial.dat" every ::::t using 1:2 title "with initial" with linespoints,\
   	"default.dat" every ::::t using 1:2 title "without initial" with linespoints # , 0.2653 title "Moses"
   # plot "initial.dat" using 1:2 with linespoints
   # plot "<(sed -n '1,3p' initial.dat)" using 1:2 title "BLEU" with linespoints


   ###########################################
   ## plot model scores
   ###########################################

   set title "model score"
   set xlabel "modification steps"
   # set key left top
   set key right bottom
   set ylabel "model score"

   set xrange [1:max]
   # set autoscale y
   set yrange [-3000:-500]
   set format x "2^{%L}"
   plot "initial-search.dat" every ::::t using 1:2 notitle with linespoints,\
   	"initial-search.dat" every ::::t using 1:3 notitle with linespoints,\
   	"initial-search.dat" every ::::t using 1:4 notitle with linespoints,\
   	"initial-search.dat" every ::::t using 1:5 notitle with linespoints,\
   	"initial-search.dat" every ::::t using 1:6 notitle with linespoints




   ###########################################
   ## plot operations
   ###########################################


   set object 1 rectangle from graph 0,0 to graph 1,1 behind fc rgb "#ffffff"

   set title "accepted operations in %"
   # set ylabel "accepted in %"
   set key below

   unset logscale x
   unset xlabel
   unset ylabel
   set yrange [0.0001:0.35]
   set ytics ("0.001" 0.001,"0.01" 0.01,"0.1" 0.1,"0.3" 0.3)

   set logscale y 2
   set autoscale x
   # set autoscale y
   set style data histogram
   # set style histogram cluster gap 1
   # set style histogram rowstacked
   set style fill solid border -1
   set boxwidth 0.9

   datfile = sprintf('operations.%d',t)
   plot datfile using 2:xtic(1) ti col, '' u 3 ti col, '' u 4 ti col

   unset logscale y
   set key inside
   set ytics auto

   unset multiplot

}
