#########################################################################################################
# FUNCTIONS
#########################################################################################################
library(RColorBrewer)
display.brewer.all()
vCols<-brewer.pal(n=10,name="Set1")

library("data.table")
readData<-function(name=""){
  D<-as.data.frame(fread(name,header = F,blank.lines.skip = T,fill = T))
  D <- data.frame(apply(D, 2, function(x) as.numeric(as.character(x))))
  names(D)<-   c("rep","year", "pop", "nPrevDS" ,"nIncDS" ,"nIncDS_fastProg" ,"nIncDS_slowProg" ,"nIncDS_relapse" ,"nIncDS_failure",
                 "nPrevDR" ,"nIncDR" ,"nIncDR_fastProg", "nIncDR_slowProg" ,"nIncDR_relapse" ,"nIncDR_resis", "nIncDR_failure",
                 "nTransDS", "nTransDR", "nTransDS_HH" ,"nTransDR_HH", 
                 "nIncHH","nIncCom","nIncDsHH","nIncDsCom",
                 "nEnterELDS" ,"nEnterELDR", "nMortalityDS","nMortalityDR" ,
                 "nVisits","nDiagDS","nDiagDR","nDST","nLTFU",
                 "nTrtDS_DS" ,"nTrtDS_DR", "nTrtDR_DR" ,
                 "nFailedDSTrt_DS","nFailedDSTrt_DR","nFailedDRTrt_DR",
                 "nNewTB","nRetreatedTB","nNewDR","nRetreatedDR",
                 "nSUSDS","nELDS","nLLDS","nRDS","nADS","nTRDS_DS","nTRDR_DS",
                 
                 "_coefTrans","_coefHHTrans","_coefComTrans","_probMaxMortalityTB",
                 "_probMaxSeekCare","_probResistance","_coefMaxInfect_DR_RelTo_DS",
                 # 
                 "durDS","nDurDS","durDR","nDurDR",
                 #
                 "prpHhPrevDS_diagDS","nHhPrevDS_diagDS",
                 "prpHhPrevDR_diagDS","nHhPrevDR_diagDS",
                 "prpHhPrevTB_diagDS","nHhPrevTB_diagDS",
                 "prpHhPrevDS_diagDR","nHhPrevDS_diagDR",
                 "prpHhPrevDR_diagDR","nHhPrevDR_diagDR",
                 "prpHhPrevTB_diagDR","nHhPrevTB_diagDR",
                 "prpHhPrevTB_diagTB","nHhPrevTB_diagTB",
                 #
                 "rngSeed"
  )
  # turn freq into rates so changes in pop size wont matter
  freqVar<-c("nPrevDS" ,"nIncDS" ,"nIncDS_fastProg" ,"nIncDS_slowProg" ,"nIncDS_relapse" ,"nIncDS_failure",
             "nPrevDR" ,"nIncDR" ,"nIncDR_fastProg", "nIncDR_slowProg" ,"nIncDR_relapse" ,"nIncDR_resis", "nIncDR_failure",
             "nTransDS", "nTransDR", "nTransDS_HH" ,"nTransDR_HH", 
             "nIncHH","nIncCom","nIncDsHH","nIncDsCom",
             "nEnterELDS" ,"nEnterELDR", "nMortalityDS","nMortalityDR" ,
             "nVisits","nDiagDS","nDiagDR","nDST","nLTFU",
             "nTrtDS_DS" ,"nTrtDS_DR", "nTrtDR_DR" ,
             "nFailedDSTrt_DS","nFailedDSTrt_DR","nFailedDRTrt_DR")
  D[, sub(".","r",freqVar)]<-(D[,freqVar]/D$pop) *100000
  
  D$pHHTrans<-(D$nTransDS_HH+D$nTransDR_HH)/(D$nTransDS+D$nTransDR)
  D$pHHInc<-D$nIncHH/(D$nIncDS+D$nIncDR)
  D$pNewDR<-D$nNewDR/D$nNewTB
  D$pRetreatedDR<-D$nRetreatedDR/D$nRetreatedTB
  
  D$pFatalityDS<-(D$nMortalityDS)/D$nIncDS  
  D[D==-Inf]<- -1
  D[D== -1000]<-0
  return (D)
}



drawplot<-function(D,tid,text=""){
  varNames<-c("pop", "rPrevDS" ,"rIncDS" ,"rIncDS_fastProg" ,"rIncDS_slowProg" ,"rIncDS_relapse" ,"rIncDS_failure",
              "rPrevDR" ,"rIncDR" ,"rIncDR_fastProg", "rIncDR_slowProg" ,"rIncDR_relapse" ,"rIncDR_resis", "rIncDR_failure",
              "rMortalityDS","rMortalityDR" ,"pFatalityDS",
              "rVisits","rDiagDS","rDiagDR","rDST","rLTFU",
              "rTrtDS_DS" ,"rTrtDS_DR", "rTrtDR_DR" ,
              "durDS","durDR",
              "prpHhPrevDS_diagDS", 
              "prpHhPrevDR_diagDS",
              "prpHhPrevTB_diagDS",
              "prpHhPrevDS_diagDR",
              "prpHhPrevDR_diagDR",
              "prpHhPrevTB_diagDR",
              "prpHhPrevTB_diagTB",
              "nNewDR","nRetreatedDR",
              "pHHTrans","pHHInc","pNewDR","pRetreatedDR")
  jpeg(paste("summaryPlots",tid,".jpeg",sep=""),width =3000,height = 3000,res = 200)
  par(mfrow=c(7,6))
  if (length(unique(D$rep))==1) {
    lapply(c(1:length(varNames)),function(i){
      varname=varNames[i]
      plot(x=D$year,y = D[,varname],type="l",col=vCols[1],lwd=2,main=paste(i,"-",varname,sep="" ),ylab="")
      })
  }else{
    reps=(unique(D$rep))
    DD<-D[D$rep==reps[1],]
    lapply(c(1:length(varNames)),function(i){
      varname=varNames[i]
      plot(x=DD$year,y = DD[,varname],type="l",col=vCols[1],lwd=2,main=paste(i,"-",varname,sep="" ),
           ylim=c(min(D[,varname]),max(D[,varname])),ylab="")
      lapply(c(2: length(reps)),function(r){
        DD<-D[D$rep==reps[r],]
        lines(x=DD$year,y =DD[,varname],col=vCols[r] )
      })})
  }
  legend("topleft",legend = unique(D$rep),fill=vCols,bty = "n")
  mtext('text is here', outer = T)
  dev.off()
}

D<-readData(name = "newOut1")
drawplot(D,1)


D<-readData(name = "c.src/out_mcmc_0")
drawplot(D,5)
# 
# jpeg("summaryPlots.jpeg",width =4000,height = 4000,res = 200)
# par(mfrow=c(10,12))
# lapply(c(1:ncol(D)),function(x){
#   plot(x=D$year,y = D[,x],type="l",col=vCols[2],lwd=2,main=names(D)[x] )})
# dev.off()

# D<-readData("new_mcmc")
# table(D$rep)



# 
# varNames<-c("rIncDS","rMortalityDS","rPrevDS","durDS","pFatalityDS",
#             "pNewDR","pRetreatedDR",
#             "prpHhPrevTB_diagTB")
# targets<-c(292, 0, 0, 14.4, 0.17, 
#            .03,.14,
#            0)
# names(targets)<-varNames
# par(mfrow=c(3,3))
# lapply(varNames,function(x){
#   plot(x=D$year,y = D[,x],type="l",col=vCols,lwd=2,main=x )
#   abline(h = targets[x],col="red")
# })
# 
