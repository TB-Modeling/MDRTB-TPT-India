read.sim.data<-function(name=""){
  D<-as.data.frame(fread(name, header = F, blank.lines.skip = T, fill = T))
  D<- data.frame(apply(D, 2, function(x) as.numeric(as.character(x))))
  names(D) <- c("rep","year", "pop", "nPrevDS" ,"nIncDS" ,"nIncDS_fastProg" ,"nIncDS_slowProg" ,"nIncDS_relapse" ,"nIncDS_failure",
                "nPrevDR" ,"nIncDR" ,"nIncDR_fastProg", "nIncDR_slowProg" ,"nIncDR_relapse" ,"nIncDR_resis", "nIncDR_failure",
                "nTransDS", "nTransDR", "nTransDS_HH" ,"nTransDR_HH",
                "nIncHH","nIncCom","nIncDsHH","nIncDrHH",
                "nEnterELDS" ,"nEnterELDR", "nMortalityDS","nMortalityDR" ,
                "nVisits","nDiagDS","nDiagDR","nDST","nLTFU",
                "nTrtDS_DS" ,"nTrtDS_DR", "nTrtDR_DR" ,
                "nFailedDSTrt_DS","nFailedDSTrt_DR","nFailedDRTrt_DR",
                "nNewTB","nRetreatedTB","nNewDR","nRetreatedDR",
                "nSUSDS","nELDS","nLLDS","nRDS","nADS","nTRDS_DS","nTRDR_DS",
                
                #CALIB PARAMS
                "_coefTrans",
                "_coefHHTrans",
                "_coefComTrans",
                "_probMaxMortalityTB",
                "_probMaxSeekCare",
                "_probResistance",
                "_coefMaxInfect_DR_RelTo_DS",
                "_coefCom2000ReducPerc", 
                "_coefHH2000ReducPerc",
                
                #BETA PARAMS
                "_coefInfec_0to10", 
                "_probPretreatmentLTF_DSTB", 
                "_probPretreatmentLTF_DRTB",
                "_probDRTrtFailure", 
                "_probDSTrtFailure", 
                "_probRelapse_DS", 
                "_probRelapse_DR",
                "_probSlowProg", 
                "_probSpontResolution", 
                "_coefImmunityLatent",   
                "_coefInfect_trtFailure", 
                "_probSuccessDStrt_DRTB", 
                "_coefFastProg",
                #
                "durDS","nDurDS","durDR","nDurDR",
                #
                "nHhTraced_diagADS", "nHhMembersTraced_diagADS","nHhPrevADS_diagADS", "nHhPrevADR_diagADS", 
                "nHhTraced_diagADR", "nHhMembersTraced_diagADR","nHhPrevADS_diagADR", "nHhPrevADR_diagADR", 
                
                "nHhPrevLADS_diagADS", 
                "nHhPrevADS_hhOriginated_diagADS","nHhPrevLADS_hhOriginated_diagADS",
                
                #
                "_hhct_probADRcase_followup",
                "_hhct_testSens_atb",
                "_hhct_testSens_ltb",
                "_hhct_ptCoverage_0To5",
                "_hhct_ptCoverage_0To15",
                "_pt_regimen",
                "_pt_probSuccess_rec",
                "_pt_probSuccess_latent",
                "_pt_duration",
                "_pt_protAgainstReinfection",
                #
                "nHhct_hhScreened",
                "nHhct_atbDiagnosed",
                "nHhct_ltbDiagnosed",
                "nPt_started",
                "nPt_completed",
                "nOnPT",
                "nHasBeenOnPT",
                "nIncDS_onPt",
                "nIncDS_postPt",
                "nIncDR_onPt",
                "nIncDR_postPt",
                
                #
                "nEltbToAtb_LT5yrs",
                "prop_LTBI",
                "rngSeed"
  )
  #exclude failures from Incidence:
  D$nIncDS=D$nIncDS-D$nIncDS_failure
  D$nIncDR=D$nIncDR-D$nIncDR_failure
  
  # # turn freq into rates so changes in pop size wont matter
  # freqVar<-c("nPrevDS" ,"nIncDS" ,"nIncDS_fastProg" ,"nIncDS_slowProg" ,"nIncDS_relapse" ,"nIncDS_failure",
  #            "nPrevDR" ,"nIncDR" ,"nIncDR_fastProg", "nIncDR_slowProg" ,"nIncDR_relapse" ,"nIncDR_resis", "nIncDR_failure",
  #            "nTransDS", "nTransDR", "nTransDS_HH" ,"nTransDR_HH", 
  #            "nIncHH","nIncCom","nIncDsHH","nIncDrHH",
  #            "nEnterELDS" ,"nEnterELDR", "nMortalityDS","nMortalityDR",
  #            "nVisits","nDiagDS","nDiagDR","nDST","nLTFU",
  #            "nTrtDS_DS" ,"nTrtDS_DR", "nTrtDR_DR" ,
  #            "nFailedDSTrt_DS","nFailedDSTrt_DR","nFailedDRTrt_DR",
  #            "nSUSDS","nELDS","nLLDS","nRDS","nADS","nTRDS_DS","nTRDR_DS")
  # D[, sub(".", "r", freqVar)]<-(D[, freqVar]/D$pop) *100000
  # 
  # #rate of DST among all visits
  # D$rDST<-D$nDST/D$nVisits
  # #rate of LTFU after diagnosis
  # D$rLTFU<-D$nLTFU/(D$nDiagDS+D$nDiagDR)
  # 
  # #prp of all transmission or incidence due to household contacts
  # D$pHHTrans<-(D$nTransDS_HH+D$nTransDR_HH)/(D$nTransDS+D$nTransDR)
  # D$pHHInc<-D$nIncHH/(D$nIncDS+D$nIncDR)
  # 
  # # prevalence of TB among households at diagnosis:
  # D$pHhPrevADS_diagADS=D$nHhPrevADS_diagADS/D$nHhMembersTraced_diagADS;
  # D$pHhPrevLADS_diagADS=D$nHhPrevLADS_diagADS/D$nHhMembersTraced_diagADS;
  # D$pHhPrevADS_hhOriginated_diagADS=D$nHhPrevADS_hhOriginated_diagADS/D$nHhPrevADS_diagADS;
  # D$pHhPrevLADS_hhOriginated_diagADS=D$nHhPrevLADS_hhOriginated_diagADS/D$nHhPrevLADS_diagADS;
  # #
  # D$pHhPrevATB_diagADS=(D$nHhPrevADS_diagADS+D$nHhPrevADR_diagADS)/D$nHhMembersTraced_diagADS;
  # D$pHhPrevATB_diagADR=(D$nHhPrevADS_diagADR+D$nHhPrevADR_diagADR)/D$nHhMembersTraced_diagADR;
  # D$pHhPrevATB_diagATB=(D$nHhPrevADS_diagADS+
  #                         D$nHhPrevADR_diagADS+
  #                         D$nHhPrevADS_diagADR+
  #                         D$nHhPrevADR_diagADR)/(D$nHhMembersTraced_diagADS+D$nHhMembersTraced_diagADS);
  # 
  # #proportion of incidence due to different paths:
  # D$propIncDS_fastProg=D$nIncDS_fastProg/D$nIncDS
  # D$propIncDR_fastProg=D$nIncDR_fastProg/D$nIncDR
  # D$propIncDR_resis=D$nIncDR_resis/D$nIncDR
  # D$propIncDR_failure=D$nIncDR_failure/D$nIncDR
  # 
  # #proportion of treatments failed:
  # D$prpDStrt_DR_failed<-D$nFailedDSTrt_DR/D$nTrtDS_DR
  # D$prpDStrt_DS_failed<-D$nFailedDSTrt_DS/D$nTrtDS_DS
  # D$prpDRtrt_DR_failed<-D$nFailedDRTrt_DR/D$nTrtDR_DR
  # 
  # #proportion of DR among new and retreated patients 
  # D$pNewDR<-D$nNewDR/D$nNewTB
  # D$pRetreatedDR<-D$nRetreatedDR/D$nRetreatedTB
  # 
  # D$pFatalityDS<-(D$nMortalityDS)/D$nIncDS 
  # D[D==-Inf]<- -1
  # D[D== NA]<- 0
  # D[D== -1000]<-0
  return (D)
}

read.loglik.data<-function(inputName){
  D<-as.data.frame(fread(inputName,header = T,data.table = T))
  #rename the log.lik outcomes
  names(D)<-c("rep",
              "inc.dsdrtb.2018",
              "inc.dsdrtb.red",
              "dur.dstb.originalRange",
              "dur.dstb.increasedRange",
              "mort.dsdrtb.2018.originalRange", # 33[30-36] per original data
              "mort.dsdrtb.2018.increasedRange", #33[23-43] assumig a wide range
              "inc.drtb.2018",
              "rrNew",
              "rrRetreated",
              "pHhPrevADS_diagADS"
  )
  return(D)
}


read.indiv.data<-function(name=""){
  D<-as.data.frame(fread(name, header = F, blank.lines.skip = T, fill = T))
  D<- data.frame(apply(D, 2, function(x) as.numeric(as.character(x))))
  names(D) <- c(
    "time",
    "id",
    "age",
    "death_time ",
    "death_route ",
    "death_dsState ",
    "death_drState ",
    "DSstate",
    "DRstate",
    "lastDSstate",
    "lastDRstate",
    "hhct_indexCase",
    "hhct_hhMember",
    "hhct_time",
    "hhct_hhID",
    "hhct_hhSize",
    "hhct_nkidsLt6InHH",
    "hhct_nkidsLt16InHH",
    "hhct_hhPrevLDS",
    "hhct_hhPrevLDR",
    "hhct_hhPrevADS",
    "hhct_hhPrevADR",
    "hhct_age",
    "hhct_dsState",
    "hhct_drState",
    "hhct_diagnosedWithADS",
    "hhct_diagnosedWithADR",
    "hhct_diagnosedWithLDS",
    "hhct_diagnosedWithLDR",
    "hhct_diagnosedWithRecDS",
    "hhct_diagnosedWithRecDR",
    "bHasBeenOnPt",
    "bIsOnPt",
    "pt_start_time",
    "pt_end_time",
    "pt_regimen",
    "pt_successCode_latent",
    "pt_successCode_rec",
    "postPt_first_dsInc_time",
    "postPt_first_dsInc_lastDsState",
    "postPt_first_dsInc_transRoute",
    "postPt_first_drInc_time",
    "postPt_first_drInc_lastDrState",
    "postPt_first_drInc_transRoute",
    "tid")
  return(D)
}
