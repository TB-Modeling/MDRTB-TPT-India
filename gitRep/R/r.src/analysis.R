
#path to find data files
data_path = '~/Dropbox/Simulations/MDRTB/mdrtb1/gitRep/c.src//'
setwd(data_path) #should set this to WD so we can access the files.

#path to save new outputs from this code
r_path="~/Dropbox/Simulations/MDRTB/mdrtb1/gitRep/r.src/PT/"

vCols<-c("red","cyan3","cyan4")
########################################################################################################################
library(data.table)
library("gtools")
library("abind")
library("logitnorm")

D<-read.sim.data("new_annualOutputs")
D[1,]
par(mfrow=c(5,5))
lapply(names(D),function(var){
  DD<-D[ D$"_pt_regimen"==0, ]
  plot(x=DD$year,y=DD[,var],type="l",col=vCols[1],main= var,lwd=2)
  DD<-D[ D$"_pt_regimen"==1, ]
  lines(x=DD$year,y=DD[,var],type="l",col=vCols[2],main= var,lwd=2)
  DD<-D[ D$"_pt_regimen"==2, ]
  lines(x=DD$year,y=DD[,var],type="l",col=vCols[3],main= var,lwd=2)
})
D[1,]



D<-read.indiv.data("out_pt_individualOutputs_00488270")
D
D[1,]
