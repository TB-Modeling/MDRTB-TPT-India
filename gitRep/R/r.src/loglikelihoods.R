# log(dnorm(  (log(inc.dstb.1970) - log(292))/ ((log(341)-log(199))/4)   )), #same incidence target from 1970 and 2000 (we r looking for equilibrium)
# log(dnorm(  (log(inc.dstb.2000) - log(292))/ ((log(341)-log(199))/4)   )),
# log(dnorm(  (log(inc.dstb.2018) - log(199))/ ((log(136)-log(233))/4)   )), #reduction in incidence at 2% per year
# log(dnorm(  (log(dur.dstb.2000.forward) - log(1.2*12))/ ((log(1.6 * 12)-log(.9 *12))/4)   )),
# 
# #   log(dnorm(  (logit(prpFatality.dstb.2000.forward) - logit(0.17))/ ((logit(.26)-logit(.13))/4)   )),
# log(dnorm(  (log(mort.dstb.2018) - log(33))/ ((log(30)-log(36))/4)   )),
# 
# log(dnorm(  (logit(rrNew.2013.18) - logit(0.03))/ ((logit(.02)-logit(.04))/4)   )),
# log(dnorm(  (logit(rrRetreated.2013.18) - logit(0.14))/ ((logit(.12)-logit(.16))/4)   )))

library("logitnorm") #for logit function

n=.11
var=seq(0,3,.05)
mat=lapply(var,function(x){
  return (c( (1-x)*100,
             log(dnorm(  (log(x*292) - log(292))/ ((log(320)-log(262))/4)   )),
             log(dnorm(  (log(x*1.2*12) - log(1.2*12))/ ((log(1.31 * 12)-log(1.09 *12))/4)   )),
             log(dnorm(  (log(x*33) - log(33))/ ((log(30)-log(36))/4)   )),
             log(dnorm(  (logit(x*0.03) - logit(0.03))/ ((logit(.027)-logit(.033))/4)   )) 
  ))
})
mat<-matrix(unlist(mat),ncol=5,byrow = T)
mat
par(mfrow=c(1,1))
plot(x = mat[,1],y=mat[,2],type="l",xlab="%difference from mean",ylab="log(dnorm)", col=vCols[1],ylim=c(-400,200),lwd=2)  
lapply(c(3:5),function(x){
  lines(x = mat[,1],y=mat[,x],col=vCols[x-1],lwd=2)
})
legend("topleft",legend = c(
  "log nIncDSTB",
  "log DurDSTB",
  "log nMortalityDSTB",
  "logit pPrevTbInHH"),fill = vCols)

