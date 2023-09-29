'Utility functions'
'building transparent colors for shading purposes' 
makeTransparent<-function(someColor, alpha=100){
  newColor<-col2rgb(someColor)
  apply(newColor, 2, function(curcoldata){
    rgb(red=curcoldata[1], green=curcoldata[2],
        blue=curcoldata[3],alpha=alpha, maxColorValue=255)})
}
'returns summary statistics'
returnStats<-function(D,limits=c(.25,.75)){
  D[D==Inf]<-NA;D[D==-Inf]<-NA;
  v<- c(mean(D,na.rm = T),median(D,na.rm = T),
        quantile(D,limits,na.rm = T))
  names(v)<-c("mean","median","q1","q2")
  return(round(v,5))
}
'Elapsed time in seconds
@param dt(difftime): A difftime object; can also accept be an expression 
of the difference of two Sys.time objects (e.g. `a`, `b`) of the form 
time.elapsed.seconds(a - b).
@returns numeric: Integer representing number of seconds elapsed'
time.elapsed.seconds <- function(dt) {
  as.integer(dt)
}
###############################################################
'Summarize by 1 or 2 or 3 factor variables
Gets unique set of `factors`` from `factorName` col in data.frame `D`. Then 
creates a new data.frame `S` comprised of 1 row for each combination of 
`years`*`factors` (true?)., where each field contains the `mean`` or `quantiles`
of the values from original data.frame `D`.
@param D(data.frame): Dataset
@param factorName(char): Name of factor/column to summarize
@returns data.frame: Summarized dataset'

summarize.mean.1fac<-function(D,fac1="year"){
  S<-lapply(c(min(D[,fac1]):max(D[,fac1])),function(x){
    M<-D[ (D[,fac1]==x), ]
    return (apply(M,2,mean,na.rm = T))
  });
  S<-as.data.frame(do.call(rbind,S))
  names(S)<-names(D)
  return(S)
}

summarize.quantile.1fac<-function(D,fac1,q){
  S<-lapply(c(min(D[,fac1]):max(D[,fac1])),function(x){
    M<-D[ (D[,fac1]==x), ]
    return (apply(M,2,quantile,q,na.rm = T))
  });
  S<-do.call(rbind,S)
  names(S)<-names(D)
  S[is.na(S)]<--1
  S[S==Inf]<--1
  return(S)
}

summarize.quantile.2fac<-function(D,fac1,fac2,q){
  S<-lapply(c(min(D[,fac1]):max(D[,fac1])),function(x){
    M<-D[ (D[,fac1]==x), ]
    SS<-lapply(c(min(S[,fac2]):max(S[,fac2])),function(y){
      MM<-S[(S[,fac2]==y), ]
      return (apply(MM,2,quantile,q,na.rm = T))
    })
    SS<-do.call(rbind,SS)
    return (SS)
  })
  S<-do.call(rbind,S)
  names(S)<-names(D)
  S[is.na(S)]<--1
  S[S==Inf]<--1
  return(S)
}
summarize.quantile.3fac<-function(D,fac1,fac2,fact3,q){
  S<-lapply(c(min(D[,fac1]):max(D[,fac1])),function(x){
    M<-D[ (D[,fac1]==x), ]
    SS<-lapply(c(min(S[,fac2]):max(S[,fac2])),function(y){
      MM<-S[(S[,fac2]==y), ]
      SSS<-lapply(c(min(SS[,fac3]):max(SS[,fac3])),function(z){
        MMM<-SS[(SS[,fac3]==z), ]
        return (apply(MMM,2,quantile,q,na.rm = T))
      })
      SSS<-do.call(rbind,SSS)
      return (SSS)
    })
    SS<-do.call(rbind,SS)
    return (SS)
  })
  S<-do.call(rbind,S)
  names(S)<-names(D)
  S[is.na(S)]<--1
  S[S==Inf]<--1
  return(S)
}
summarize.mean.year.factor<-function(D,factorName){
  factors = unique(D[,factorName])
  S<-lapply(c(min(D$year):max(D$year)),function(x){
    D1<-D[(D$year==x), ]
    SS<-lapply(factors,function(c){
      D2<-D1[(D1[,factorName] == c) , ]
      return (apply(D2,2,mean,na.rm = T))
    })
    SS<-as.data.frame(do.call(rbind,SS))
    return(SS)
  })  
  S<-as.data.frame(do.call(rbind,S))
  names(S)<-names(D)
  return(S)
}




