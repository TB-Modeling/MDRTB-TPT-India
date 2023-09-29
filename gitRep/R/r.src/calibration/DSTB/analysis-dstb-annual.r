#########################################################################################################
# FUNCTIONS
#########################################################################################################
library(RColorBrewer)
display.brewer.all()
vCols<-brewer.pal(n=10,name="Set2")

library("data.table")
readData<-function(name="newPop20m"){
  D<-as.data.frame(fread(name,header = F))
  D <- data.frame(apply(D, 2, function(x) as.numeric(as.character(x))))
  names(D)<-  c("rep","year", "pop", "nPrevDS" ,"nIncDS" ,"nIncDS_fastProg" ,"nIncDS_slowProg" ,"nIncDS_relapse" ,"nIncDS_failure",
                "nPrevDR" ,"nIncDR" ,"nIncDR_fastProg", "nIncDR_slowProg" ,"nIncDR_relapse" ,"nIncDR_resis", "nIncDR_failure",
                "nTransDS", "nTransDR", "nTransDS_HH" ,"nTransDR_HH", 
                "nIncHH","nIncCom","nIncDsHH","nIncDsCom",
                "nEnterELDS" ,"nEnterELDR", "nMortalityDS","nMortalityDR" ,
                "nVisits","nDiagDS","nDiagDR","nDST","nLTFU",
                "nTrtDS_DS" ,"nTrtDS_DR", "nTrtDR_DR" ,
                "nFailedDSTrt_DS","nFailedDSTrt_DR","nFailedDRTrt_DR",
                "nNewTB","nRetreatedTB","nNewDR","nRetreatedDR",
                "nSUSDS","nELDS","nLLDS","nRDS","nADS","nTRDS_DS","nTRDR_DS",
                "_coefTrans","_coefHHTrans","_coefComTrans","_probMaxMortalityTB","_probMaxSeekCare","_probResistance","_coefMaxInfect_DR_RelTo_DS"
  )
  D$pHHTrans<-(D$nTransDS_HH+D$nTransDR_HH)/(D$nTransDS+D$nTransDR)
  D$pHHInc<-D$nIncHH/(D$nIncDS+D$nIncDR)
  D$pNewDR<-D$nNewDR/D$nNewTB
  D$pRetreatedDR<-D$nRetreatedDR/D$nRetreatedTB
  return (D)
}

drawPlot<-function(data){
  plot(data,type="l",col=vCols,lwd=2,main=names(data) )}
#########################################################################################################
# ANALYSIS
#########################################################################################################
# setwd("~/Dropbox/Simulations/MDRTB/mdrtb/Build/Products/Debug/")
# setwd("~/Dropbox/Simulations/MDRTB/mdrtb/gitRep/src/")
getwd()

#read file:
D<-readData("src/outout_annualTbSummary_3")

D[1,]
par(mfrow=c(2,1))
drawPlot(D$nIncDS);drawPlot(D$nPrevDS)
drawPlot(D$pHHTrans);drawPlot(D$pHHInc)
drawPlot(D$nMortalityDS);drawPlot(D$nDiagDS)

Q<-unlist(fread("output_durDSTB_3",sep = " ",header = F))
hist(Q,breaks = 100)
median(Q);range(Q)
