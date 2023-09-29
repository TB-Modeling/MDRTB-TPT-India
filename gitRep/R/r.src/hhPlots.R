
hhSummary.data = read.csv("../../src/output_annualHhSummary_3.csv")
#hhFollow.data = read.csv("../../src/output_followHH_3_20.csv", header=FALSE, skip=1)

run_years = hhSummary.data["Year"]
hh_count = hhSummary.data["Number.of.HH"]
year_count = 499

#Household Size over time
for (i in 1:dim(run_years)[1]) {
  year = run_years[i,]
  #check_year = 1800
  total_hh_year = hh_count[i,]

  year_slice = hhSummary.data[ hhSummary.data$Year == year, grep("nSize", names(hhSummary.data))]
  incidence = hhSummary.data[ hhSummary.data$Year == year, "Incidence"]
  prevalence = hhSummary.data[ hhSummary.data$Year == year, "Prevalence"]
  
  png( file = sprintf("~/dev/parastu/images/householdSize_%d.png",year))
  plot (0 : 33, 
        year_slice / total_hh_year, 
        type="s", 
        xlab="Household Size", 
        ylab="Proportion of Total HH", 
        main=sprintf("Year : %d, Incidence : %.2f, Prevalence : %d",year, incidence, prevalence),
        ylim=c(0,1))
  dev.off()
}

#Number of all kids households
all_kids = hhSummary.data["HH.of.all.kids"]
plot (0:499,
      all_kids[,],
      type="l",
      xlab="Year",
      ylab="Households of all Kids",
      main="Number of all children (under 21) households",
      ylim=c(0,25))

#Median HH size
medianHHSize = hhSummary.data["Median.HH.Size"]
plot(0:499,
     medianHHSize[,],
     type="l",
     xlab="Year",
     ylab="HH size",
     main="Median HH Size over time",
     ylim=c(0,10))

#Number of HH over time
countHH = hhSummary.data["Number.of.HH"]
plot(0:499,
     countHH[,],
     type="l",
     xlab="Year",
     ylab="Number of HH",
     main="Number of HH over time",
     ylim=c(0,5100))

#Number of all kid households as a percentage of total households
all_kids_over_totalHH = all_kids / countHH
plot(0:499,
     all_kids_over_totalHH[,],
     type="l",
     xlab="Year",
     ylab="Proportion of all kids households",
     main="Proportion of all kids (< 21) households over time",
     ylim=c(0,0.05))
