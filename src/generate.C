// Macro to generate macros for the ALICE TRAIN
// Authors: Andrea Gheata, Jan Fiete Grosse-Oetringhaus, Costin Grigoras
//
// example:
// aliroot -b -q generate.C'("/alice/data/2010/LHC10h/000139510/ESDs/pass2", "AliESDs.root", kTRUE, 1)'
//
// requires env variables 
//   TRAIN_TESTDATA which contains folder where to copy test data
//   ALIROOT_VERSION_SHORT with the package name of the desired aliroot version (e.g. v4-21-33-AN)
//   ALIPHYSICS_VERSION_SHORT with the package name of the desired aliphysics version (e.g. v4-21-33-AN)

void generate(const char* trainFolder = "test/test", Bool_t copyData = kTRUE, const char *module = "")
{
   TString dataBasePath(gSystem->Getenv("TEST_DIR"));
   TString dataAnchor(gSystem->Getenv("FILE_PATTERN"));
   Int_t nFiles = TString(gSystem->Getenv("TEST_FILES_NO")).Atoi();
   Int_t splitMaxInputFileNumber = TString(gSystem->Getenv("SPLIT_MAX_INPUT_FILE_NUMBER")).Atoi();
   Int_t maxMergeFiles = TString(gSystem->Getenv("MAX_MERGE_FILES")).Atoi();
   Int_t debugLevel = TString(gSystem->Getenv("DEBUG_LEVEL")).Atoi();
   Int_t ttl = TString(gSystem->Getenv("TTL")).Atoi();
   TString excludeFiles(gSystem->Getenv("EXCLUDE_FILES"));
   TString friendChainNames(gSystem->Getenv("FRIEND_CHAIN_NAMES"));
   TString friendChainLibraries(gSystem->Getenv("FRIEND_CHAIN_LIBRARIES"));
   TString additionalpackages(gSystem->Getenv("ADDITIONAL_PACKAGES"));
   TString periodName(gSystem->Getenv("PERIOD_NAME"));
  
   const char *train_name="lego_train";
   
   Bool_t generateProduction = kFALSE;
   
   if (strcmp(module, "__ALL__") == 0)
     module = "";
   
   if (strcmp(module, "__TRAIN__") == 0)
   {
     generateProduction = kTRUE;
     module = "";
   }

   gSystem->Load("libANALYSIS");
   gSystem->Load("libANALYSISalice");
   
   if (atoi(gSystem->Getenv("ADDTASK_NEEDS_ALIEN")) == 1)
   {
     Printf("Connecting to AliEn...");
     TGrid::Connect("alien:");
   }

   TObjArray *arr = AliAnalysisTaskCfg::ExtractModulesFrom("MLTrainDefinition.cfg");
   
   Printf(">>>>>>> Read train configuration");
   arr->Print();
   
   // add baseline task if requested
   if (strcmp(module, "__BASELINE__") == 0)
   {
     taskCfg = new AliAnalysisTaskCfg("__BASELINE__");
     taskCfg->SetMacroName("$ALICE_ROOT/ANALYSIS/macros/train/AddTaskBaseLine.C");
     taskCfg->SetDataTypes("ESD AOD MC");
     arr->Add(taskCfg);
   }
   
   AliAnalysisAlien *plugin = new AliAnalysisAlien(train_name);
   // General plugin settings here
   plugin->SetProductionMode();

   plugin->SetAPIVersion("V1.1x");

   // libraries because we start with root!
   const char* rootLibs = "libVMC.so libPhysics.so libTree.so libMinuit.so libProof.so libSTEERBase.so libESD.so libAOD.so";
   plugin->SetAdditionalRootLibs(rootLibs);

   TString alirootVersion(gSystem->Getenv("ALIROOT_VERSION_SHORT"));
   TString aliphysicsVersion(gSystem->Getenv("ALIPHYSICS_VERSION_SHORT"));
   if (alirootVersion.Length() == 0 && aliphysicsVersion.Length() == 0)
   {
      Printf("ERROR: ALIROOT_VERSION_SHORT and ALIPHYSICS_VERSION_SHORT not set. Exiting...");
      return;
   }
   
   TString jobTag(trainFolder);
   tokens = jobTag.Tokenize("/");
   plugin->SetJobTag(Form("%s/%s", tokens->At(0)->GetName(), tokens->At(1)->GetName()));
   delete tokens;

   plugin->SetROOTVersion(gSystem->Getenv("ROOT_VERSION_SHORT"));
   plugin->SetAliROOTVersion(alirootVersion);
   if (aliphysicsVersion.Length() > 0)
      plugin->SetAliPhysicsVersion(aliphysicsVersion);
   plugin->SetMaxMergeFiles(maxMergeFiles);
   plugin->SetTTL(ttl);
   plugin->SetAnalysisMacro(Form("%s.C", train_name));
   plugin->SetValidationScript("validation.sh");
   
   plugin->SetRegisterExcludes(excludeFiles + " AliAOD.root");
   if (friendChainNames.Length() > 0)
   {
     if (friendChainLibraries.Length () > 0)
       plugin->SetFriendChainName(friendChainNames, friendChainLibraries);
     else
       plugin->SetFriendChainName(friendChainNames);
   }
   else if (friendChainLibraries.Length() > 0) {
     Printf("friendChainLibraries: %s", friendChainLibraries.Data());
     plugin->SetAdditionalRootLibs(Form("%s %s", rootLibs, friendChainLibraries.Data()));
   }
    
   // jemalloc
   additionalpackages += " jemalloc::v3.6.0"; 
   plugin->AddExternalPackage(additionalpackages);
   
   plugin->SetJDLName(Form("%s.jdl", train_name));
   plugin->SetExecutable(Form("%s.sh", train_name));
   plugin->SetSplitMode("se");
   
   plugin->SetGridWorkingDir(TString("/alice/cern.ch/user/a/alitrain/") + trainFolder);

   if (!generateProduction)
    plugin->SetKeepLogs(kTRUE);
   
   plugin->SetMergeViaJDL();
   
   if (alirootVersion >= "v5-05-72-AN" || aliphysicsVersion.Length() > 0)
     plugin->SetMergeAOD();
   
   TString dataFolder(gSystem->Getenv("TRAIN_TESTDATA"));
   if (dataFolder.Length() == 0)
   {
      Printf("ERROR: TRAIN_TESTDATA not set. Exiting...");
      return;
   }
 
 
   if (dataBasePath.Length() > 0)
   {
     TString archiveName = "root_archive.zip";
     if (atoi(gSystem->Getenv("AOD")) == 2||atoi(gSystem->Getenv("AOD")) == 7)
       archiveName = "aod_archive.zip";
     if (friendChainNames.Length() > 0)
       archiveName += ";" + friendChainNames;
    
     if (nFiles <= 0)
       nFiles = 1;
     
     TString dataTag;
     dataTag.Form("%s_%s_%s_%d", dataBasePath.Data(), archiveName.Data(), dataAnchor.Data(), nFiles);

     dataTag.ReplaceAll(";", "__");
     dataTag.ReplaceAll("/", "__");
     dataTag.ReplaceAll(".root", "");
     dataTag.ReplaceAll(".zip", "");
    
     TString dataFileName(dataTag + ".txt");
     
     Printf("\n>>>> Test files are taken from: %s", dataFileName.Data());
   
     plugin->SetFileForTestMode(Form("%s/%s", dataFolder.Data(), dataFileName.Data()));
     plugin->SetNtestFiles(nFiles);

     // Is MC only?
     if (atoi(gSystem->Getenv("AOD")) == 3) {
       Printf(">>>> Expecting MC only production");
       plugin->SetUseMCchain();
     }
   
     // Copy dataset locally
     if (copyData) {
       TString cdir = gSystem->WorkingDirectory();
       gSystem->cd(dataFolder);
       
       Printf("Obtaining a lock for this dataset. Only one process can do that at a time, if another test run is copying files, this might block for some time...");
       TString lockFilePath;
       lockFilePath.Form("%s/train-daemon-lock_%s", dataFolder.Data(), dataTag.Data());
       
       TLockFile lockFile(lockFilePath, 3600*24);
       Printf("Lock obtained...");

       // check if files are already copied
       if (gSystem->AccessPathName(dataFileName)) {
	 if (!gGrid) {
	   Printf("Connecting to AliEn...");
	   TGrid::Connect("alien:");
	 }
	 
	 gSystem->Exec(Form("touch %s/../downloading_input_files", cdir.Data()));
         
	 Printf("Getting ready to copy test files locally...");

	 // special treatment for non-officially produced productions	
	 Bool_t specialSet = kFALSE;
	 if (periodName == "AMPT_LHC12g6" || periodName == "AMPT_LHC12c3")
	   specialSet = kTRUE;

	 Bool_t result = kFALSE;

	 if (specialSet) {
	   result = plugin->CopyLocalDataset(dataBasePath, "Kinematics.root", nFiles, dataFileName, "", dataTag);
	   if (!plugin->CopyLocalDataset(dataBasePath, dataAnchor, nFiles, dataFileName, "", dataTag))
	     result = kFALSE;
	 } else {
           // check for merged AODs first
           // NOTE commented out because we have too large merged AODs (~4 GB, e.g. LHC13f pass 4) and then the test takes forever
           //if (atoi(gSystem->Getenv("AOD")) == 2||atoi(gSystem->Getenv("AOD")) == 7)
           //  result = plugin->CopyLocalDataset(dataBasePath + "/AOD", dataAnchor, nFiles, dataFileName, archiveName, dataTag);
           if (!result)
             result = plugin->CopyLocalDataset(dataBasePath, dataAnchor, nFiles, dataFileName, archiveName, dataTag);
         }

	 if (!result) {
	   gSystem->Unlink(dataFileName);
	   gSystem->Exec(Form("rm -rf %s", dataTag.Data()));
	   gSystem->Exec(Form("rm %s/../downloading_input_files", cdir.Data()));
	   Printf("ERROR: Could not copy test files. Exiting...");
	   return;
	 }
	
	 // check file integrity
	 TString steerFolder(gSystem->Getenv("TRAIN_STEER"));
	 if (steerFolder.Length() == 0)	{
	   Printf("ERROR: TRAIN_STEER not set. Exiting...");
	   gSystem->Exec(Form("rm %s/../downloading_input_files", cdir.Data()));
	   return;
	 }

	 if (!specialSet)
	   gSystem->Exec(Form("%s/check-zip-files.sh %s", steerFolder.Data(), dataFileName.Data()));

	 gSystem->Exec(Form("rm %s/../downloading_input_files", cdir.Data()));
       } else {
	 // mark files as used (for cleanup)
	 gSystem->Exec(Form("touch %s", dataFileName.Data()));
       }

       // CINT does not release the lock quickly enough and if we have a crash below the file stays. That is unfortunate. Deleting the file manually.
       gSystem->Unlink(lockFilePath);
      
       gSystem->cd(cdir);
     }
   }
   
   // execute custom configuration
   Int_t error = 0;
   gROOT->Macro("globalvariables.C", &error);
   if (error != 0)
   {
      Printf("ERROR: globalvariables.C was not executed successfully...");
      return;
   }
   
   // Load modules here
   plugin->AddModules(arr);
   plugin->CreateAnalysisManager("train","handlers.C");
   
   // specific if data is processed or MC is generated on the fly
   if (atoi(gSystem->Getenv("AOD")) == 100) { // MC production
     Long64_t totalEvents = TString(gSystem->Getenv("GEN_TOTAL_EVENTS")).Atoll();

     //Printf("%lld %d", totalEvents, splitMaxInputFileNumber);

     Long64_t neededJobs = totalEvents / splitMaxInputFileNumber;

     plugin->SetMCLoop(true);
     plugin->SetSplitMode(Form("production:1-%d", neededJobs)); 
     plugin->SetNMCjobs(neededJobs);
     plugin->SetNMCevents((generateProduction) ? splitMaxInputFileNumber : nFiles);
     plugin->SetExecutableCommand("aliroot -b -q"); 

     plugin->AddDataFile("dummy.xml"); // TODO to be removed
     // /alice/cern.ch/user/a/alitrain/
     plugin->SetGridOutputDir(TString("$1/") + trainFolder + "/$4");

     plugin->SetKeepLogs(kTRUE);
   } else { // Data, ESD/AOD
     plugin->SetSplitMaxInputFileNumber(splitMaxInputFileNumber);

     // E.g. /alice/cern.ch/user/p/pwg_cf/Devel_2/72_20120928-1346/$4/lego_train_input.xml"
     // TString trainFolderReplaced(trainFolder);
     // trainFolderReplaced.ReplaceAll("PWGCF", "p/pwg_cf");
     // trainFolderReplaced.ReplaceAll("PWGPP", "p/pwg_pp");
     // trainFolderReplaced.ReplaceAll("PWGHF", "p/pwg_hf");
     // trainFolderReplaced.ReplaceAll("PWGLF", "p/pwg_lf");
     // trainFolderReplaced.ReplaceAll("PWGUD", "p/pwg_ud");
     // trainFolderReplaced.ReplaceAll("PWGDQ", "p/pwg_dq");
     // trainFolderReplaced.ReplaceAll("PWGGA", "p/pwg_ga");
     // trainFolderReplaced.ReplaceAll("PWGJE", "p/pwg_je");
     // trainFolderReplaced.ReplaceAll("PWGZZ", "a/alitrain");
     // plugin->AddDataFile(Form("/alice/cern.ch/user/%s/$4/lego_train_input.xml", trainFolderReplaced.Data()));
     plugin->AddDataFile(Form("$1/%s/lego_train_input.xml", trainFolder));

     plugin->SetGridOutputDir(TString("$1/") + trainFolder);
     // E.g. /alice/cern.ch/user/p/pwg_cf/Devel_2/72_20120928-1346/$4/#alien_counter_04i#
     // plugin->SetGridOutputDir(Form("/alice/cern.ch/user/%s/$4", trainFolderReplaced.Data()));

     plugin->SetExecutableCommand("root -b -q"); 
     plugin->SetInputFormat("xml-single");
   }
   
   AliAnalysisManager* mgr = AliAnalysisManager::GetAnalysisManager();
   mgr->SetDebugLevel(debugLevel);
   mgr->SetNSysInfo((strcmp(gSystem->Getenv("PP"), "true") == 0) ? 1000 : 40);
   
   mgr->SetFileInfoLog("fileinfo.log");

   if (alirootVersion == "v5-05-42-AN")
     mgr->SetCollectThroughput(kFALSE);

   if (generateProduction)
      plugin->GenerateTrain(train_name);
   else
      plugin->GenerateTest(train_name, module);
   
   // check for illegally defined output files
   TString validOutputFiles = gSystem->Getenv("OUTPUT_FILES");
   validOutputFiles += "," + excludeFiles;
   TString outputFiles = plugin->GetListOfFiles("out");
   tokens = outputFiles.Tokenize(",");
   
   Bool_t valid = kTRUE;
   for (Int_t i=0; i<tokens->GetEntries(); i++)
   {
     if (!validOutputFiles.Contains(tokens->At(i)->GetName()))
     {
       Printf("ERROR: Output file %s requested which is not among the defined ones for this train (%s)", tokens->At(i)->GetName(), validOutputFiles.Data());
       valid = kFALSE;
     }
   }
   delete tokens;
   
   if (!valid)
   {
     Printf(">>>>>>>>> Invalid output files requested. <<<<<<<<<<<<");
     gSystem->Unlink("lego_train.C");
   }
}
