'Plot household distribution
@param D(data.frame): Input data'
plot.hhDist <- function(D) {
  jpeg(paste(OUTPUT_DIR, "/", "hhDist.jpeg", sep=""),
       width=1000, height=1000)
  par(mfrow=c(8,5))
  
  lapply(seq(1,1000,25),function(x){
    plot(
      unlist(D[x,27:36]/D$totalHH),
      type="l",
      lwd=2,
      main=paste("Year =",x),
      ylab="prob",
      xlab="HHDist",
      col="red")
  })
  dev.off()
  
  # Build/ dir only for XCode builds? -jef
  setwd(paste(ROOT_DIR, "/Build/Products/Debug/", sep=""))
  D<-fread("temp",header = F)
  plot((unlist(D)))
  barplot(
    table(cut(
      unlist(D),
      breaks=seq(0,105,5))))
}