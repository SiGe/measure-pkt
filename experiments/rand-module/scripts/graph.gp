# Note you need gnuplot 4.4 for the pdfcairo terminal.

set terminal pdfcairo font "Gill Sans,9" linewidth 4 rounded fontscale 1.0

# Line style for axes
set style line 80 lt rgb "#808080"

# Line style for grid
set style line 81 lt 0  # dashed
set style line 81 lt rgb "#808080"  # grey

set grid back linestyle 81
set border 3 back linestyle 80 # Remove border on top and right.  These
             # borders are useless and make it harder
             # to see plotted lines near the border.
    # Also, put it in grey; no need for so much emphasis on a border.
set xtics nomirror
set ytics nomirror

#set log x
#set mxtics 10    # Makes logscale look good.

# Line styles: try to pick pleasing colors, rather
# than strictly primary colors or hard-to-see colors
# like gnuplot's default yellow.  Make the lines thick
# so they're easy to see in small plots in papers.
set style line 1 lt rgb "#A00000" lw 2 pt 1
set style line 2 lt rgb "#00A000" lw 2 pt 6
set style line 3 lt rgb "#5060D0" lw 2 pt 2
set style line 4 lt rgb "#F25900" lw 2 pt 9
set style line 5 lt rgb "#5F209B" lw 2 pt 4

set output "graph.pdf"
# Get the directory of the benchmark
set xlabel "Cache size"
set ylabel "Average (cycles)"
set key outside horizontal bottom center 
set mxtics 10
# set yrange [10:100]
set logscale x

plot 'Cuckoo-1.1-tmp.csv'       using ($12/((1024*1024/4))):($7/(1)) title 'Cuckoo-1.1'       w lp ls 2,\
     'CuckooBucket-1.1-tmp.csv' using ($12/((1024*1024/4))):($7/(1)) title 'CuckooBucket-1.1' w lp ls 5,\
     'Linear-1.1-tmp.csv'       using ($12/((1024*1024/4))):($7/(1)) title 'Linear-1.1'       w lp ls 4,\
     'Simple-1.1-tmp.csv'       using ($12/((1024*1024/4))):($7/(1)) title 'Simple-1.1'       w lp ls 5,
