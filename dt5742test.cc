#include <iostream>
#include <chrono>
#include <thread> 
#include "CAENDigitizer.h"
#include <atomic> 
#include <vector> 
#include <fstream>



#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TInterpreter.h"
#include "TMath.h"

void printTriggerMode(CAEN_DGTZ_TriggerMode_t tmode) {
  switch (tmode) {
  case CAEN_DGTZ_TRGMODE_DISABLED: {
    std::cout << "Disabled Trigger" << std::endl;
    break;
  }
  case CAEN_DGTZ_TRGMODE_EXTOUT_ONLY: {
    std::cout << "External Out Trigger Mode Only" << std::endl;
    break;
  }
  case CAEN_DGTZ_TRGMODE_ACQ_ONLY: {
    std::cout << "Acquisition Trigger Mode Only" << std::endl;
    break;
  }
  case CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT: {
    std::cout << "Acquisition Trigger Mode and External Out" << std::endl;
    break;
  }
  }


}


void printFrequency(CAEN_DGTZ_DRS4Frequency_t freq) {
    switch (freq) {
  case CAEN_DGTZ_DRS4_5GHz:
    std::cout << "DRS4 Freq: 5 GHz" << std::endl;
    break;
  case CAEN_DGTZ_DRS4_2_5GHz:
    std::cout << "DRS4 Freq: 2.5 GHz" << std::endl;
    break;
  case CAEN_DGTZ_DRS4_1GHz:
    std::cout << "DRS4 Freq: 1.0 GHz" << std::endl;
    break;
  case CAEN_DGTZ_DRS4_750MHz:
    std::cout << "DRS4 Freq: 0.75 GHz" << std::endl;
    break;
  case _CAEN_DGTZ_DRS4_COUNT_:
    std::cout << "DRS4 COUNT?" << std::endl;
    break;
  default:
    std::cout << "Unknown Freq" << std::endl;
    break;
  }
}

bool acquire(int handle) {


  CAEN_DGTZ_ErrorCode ret;
  
    // Start acquisition
  ret = CAEN_DGTZ_SWStartAcquisition(handle);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not start acquisition: " << ret << std::endl;
  }

  bool bufferFull = false; 
  for (int i=  0; i < 10; i++) { 
    std::this_thread::sleep_for(std::chrono::seconds(1)); 
    // Read Acq. Status Register
    uint32_t reg; 
    ret = CAEN_DGTZ_ReadRegister(handle, 0x8104, &reg);
    if (ret != CAEN_DGTZ_Success) {
      std::cerr << "Could not read out register: " << ret << std::endl;
    }
    std::cout << "Register value:" << std::hex << "0x" << reg << std::endl;
    if (reg & 0x10) {
      std::cout << "Buffer Full" << std::endl;
      bufferFull = true; 
      break; 
    }
  }

  ret = CAEN_DGTZ_SWStopAcquisition(handle); 
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not stop acquisition: " << ret << std::endl;
  }

  return bufferFull; 



}


int main(int argc, char **argv) {


  // Error codes
  CAEN_DGTZ_ErrorCode ret;

  // CAEN Handle, we only have one for now 
   int	handle;

  // CAEN Info 
  CAEN_DGTZ_BoardInfo_t BoardInfo;
  CAEN_DGTZ_EventInfo_t eventInfo;
  char *EventPtr = NULL;
  CAEN_DGTZ_X742_EVENT_t       *Event742=NULL;  /* custom event struct with 8 bit data (only for 8 bit digitizers) */

  // Open, get info setup digitizer
  ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB,0,0,0,&handle);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Failed to open digitizer" << std::endl;
    return -1; 
  }

  ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Failed to read info from digitizer" << std::endl;
    ret = CAEN_DGTZ_CloseDigitizer(handle); 
    return -1;
  }

  std::cout << "Model:" << BoardInfo.ModelName << std::endl <<
    "ROC FPGA:" << BoardInfo.ROC_FirmwareRel << std::endl <<
    "AMC FPGA:" << BoardInfo.AMC_FirmwareRel << std::endl <<
    "Board Family:" << BoardInfo.FamilyCode << std::endl; 

  // Reset everything 
  
  ret = CAEN_DGTZ_Reset(handle);
  std::this_thread::sleep_for(std::chrono::milliseconds(500)); 

  // Set the record length to 1024 samples (5 GS / s)
  ret = CAEN_DGTZ_SetRecordLength(handle,1024);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Failed to set record length " << ret << std::endl;
  }

  CAEN_DGTZ_TriggerMode_t tmode; 
  ret = CAEN_DGTZ_GetSWTriggerMode(handle, &tmode);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Failed to read SW Trigger mode " << ret << std::endl;
  }

  printTriggerMode(tmode); 
  
  ret = CAEN_DGTZ_GetExtTriggerInputMode(handle, &tmode);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Failed to read SW Trigger mode " << ret << std::endl;
  }
  std::cout << "External Trigger Mode: "; 
  printTriggerMode(tmode); 

  uint32_t mask; 
  ret = CAEN_DGTZ_GetGroupEnableMask(handle, &mask); 
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not read out group enable mask " << ret << std::endl;
  }
  std::cout << "Group Mask:" << std::hex << "0x" <<  mask << std::endl; 
  

  uint32_t sz; 
  ret = CAEN_DGTZ_GetRecordLength(handle, &sz); 
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not read out record length " << ret << std::endl;
  }
  std::cout << "Record Length:" << std::dec << sz << std::endl; 


  CAEN_DGTZ_DRS4Frequency_t freq;
  ret = CAEN_DGTZ_GetDRS4SamplingFrequency(handle, &freq);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not read out DRS4 freq " << ret << std::endl;
  }

  printFrequency(freq);


  //Enable Group 2
  uint32_t gemask = 0x3; 
  ret = CAEN_DGTZ_SetGroupEnableMask(handle, gemask);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not set group enable mask  " << ret << std::endl;
  }


  //Set Trigger Threshold to ~150 mV
  ret = CAEN_DGTZ_SetGroupFastTriggerThreshold(handle, 0, 23574);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not set fast trigger threshold  " << ret << std::endl;
  }

  ret = CAEN_DGTZ_SetGroupFastTriggerThreshold(handle, 1, 23574);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not set fast trigger threshold  " << ret << std::endl;
  }
  
  uint32_t TriggerThreshold = 0;
  ret = CAEN_DGTZ_GetGroupFastTriggerThreshold(handle, 0, &TriggerThreshold);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not read out fast trigger threshold  " << ret << std::endl;
  }
  std::cout << "Trig Thresh Group 0:" << TriggerThreshold << std::endl; 

  ret = CAEN_DGTZ_SetTriggerPolarity(handle, 0, CAEN_DGTZ_TriggerOnFallingEdge); 
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not set trigger polarity  " << ret << std::endl;
  }
  
  
  ret = CAEN_DGTZ_GetGroupFastTriggerThreshold(handle, 1, &TriggerThreshold);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not read out fast trigger threshold  " << ret << std::endl;
  }
  std::cout << "Trig Thresh Group 1:" << TriggerThreshold << std::endl;
  

  
  CAEN_DGTZ_TriggerMode_t fast_trig_mode;
  CAEN_DGTZ_SetFastTriggerMode(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY); 
  ret = CAEN_DGTZ_GetFastTriggerMode(handle, &fast_trig_mode);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not read out fast trigger mode  " << ret << std::endl;
  }
  if (fast_trig_mode) {
    std::cout << "Fast Trigger Enabled" << std::endl; 
  }
  else {
    std::cout << "Fast Trigger Disabled" << std::endl;

  }

  CAEN_DGTZ_EnaDis_t ft_digtz_enabled;
  ret = CAEN_DGTZ_GetFastTriggerDigitizing(handle, &ft_digtz_enabled);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not read out fast trigger digitizing enabled  " << ret << std::endl;
  }
  ft_digtz_enabled = CAEN_DGTZ_ENABLE;
  ret = CAEN_DGTZ_SetFastTriggerDigitizing(handle, ft_digtz_enabled);
  
  if (ft_digtz_enabled) {
    std::cout << "Fast Trigger Digitizing Mode Enabled" << std::endl;
  }
  else  {
    std::cout << "Fast Trigger Digitizing Mode Disabled" << std::endl;
  }
    
  uint32_t FastTrigDCOffset = 0;
  ret = CAEN_DGTZ_GetGroupFastTriggerDCOffset(handle, 0, &FastTrigDCOffset); 
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not read out fast trigger offset  " << ret << std::endl;
  }
  std::cout << "Fast Trig DC Offset:" << FastTrigDCOffset << std::endl; 

  ret = CAEN_DGTZ_GetGroupFastTriggerDCOffset(handle, 1, &FastTrigDCOffset); 
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not read out fast trigger offset  " << ret << std::endl;
  }
  std::cout << "Fast Trig DC Offset Group 2:" << FastTrigDCOffset << std::endl; 

  uint32_t numEvents;
  ret = CAEN_DGTZ_SetMaxNumAggregatesBLT(handle, 1000);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not Set Max Aggregate Readout Events  " << ret << std::endl;
  }
  
  ret = CAEN_DGTZ_GetMaxNumAggregatesBLT(handle, &numEvents);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not read Max Aggregate Events  " << ret << std::endl;
  }
  std::cout << "MaxAggregatesBLT:" << numEvents << std::endl; 


  // Enable DRS4 Corrections
  ret = CAEN_DGTZ_LoadDRS4CorrectionData(handle, CAEN_DGTZ_DRS4_5GHz); 
  ret = CAEN_DGTZ_EnableDRS4Correction(handle);
  if (ret != CAEN_DGTZ_Success) {
    std::cerr << "Could not enable drs4 corrections  " << ret << std::endl;
  }
  CAEN_DGTZ_DRS4Correction_t corrections[MAX_X742_GROUP_SIZE]; 
  ret = CAEN_DGTZ_GetCorrectionTables(handle, CAEN_DGTZ_DRS4_5GHz,  (void*)corrections);

  
							      
  
  
  char *buffer = NULL;
  uint32_t size;
  ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event742);
  ret = CAEN_DGTZ_MallocReadoutBuffer(handle,&buffer,&size);
  bool bufferFull = false;

  TFile f("output.root", "RECREATE");
  TTree tree("waves", "CV Laser Studies DRS");
  gInterpreter->GenerateDictionary("vector<vector<float>>", "vector");
  gInterpreter->GenerateDictionary("vector<float>", "vector");


  uint32_t sampleCount;
  Float_t minbin = 0; 
  std::vector<float> xs;
  std::vector< std::vector < float > > ys;
  unsigned int evtn = 0; 
  
  for (int i = 0; i < 5; i++) { 
  
    bufferFull = acquire(handle); 
  

    // Readout if we have a full buffer 
    if (bufferFull) {
    
      ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer,
			     &size);
      if (ret) {
	std::cout << "Error " << ret << std::endl;
      }
      uint32_t numevents = 0; 
      if (size != 0) {
	ret = CAEN_DGTZ_GetNumEvents(handle, buffer, size, &numevents);
	if (ret)
	  std::cout << "Error event readout" << std::endl;
	std::cout << "events:" << numevents << " buffer size:" << size << std::endl; 
      }

      // Generate our root tree


    ret = CAEN_DGTZ_GetEventInfo(handle, buffer, size, 0, &eventInfo, &EventPtr);
    if (ret)
      std::cout << "Event Info Readout failed" << std::endl; 
    

    ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event742);
    if (ret)
      std::cout << "Event Info Readout failed" << std::endl;

    if (i == 0) {
      // Setup the tree first time through 
      sampleCount = Event742->DataGroup[0].ChSize[0]; // All chs should be the same size
      xs = std::vector<float>(sampleCount);
      for (int i = 0; i < sampleCount; i++) {
	xs[i] = corrections[0].time[i]; 
      }
      
      ys =std::vector< std::vector < float > >(9, std::vector<float>(sampleCount));
      tree.Branch("time", &xs);
      tree.Branch("chs", &ys);
      tree.Branch("event", &evtn, "event/i");
      tree.Branch("min", &minbin, "min/f"); 
    }
    

    // Assuming 5GHz
    

    for (int ev = 0; ev < numevents; ev++) {
    
      if ((evtn %100) == 0)
	std::cout << "Processing event " << std::dec << evtn << std::endl; 
      ret = CAEN_DGTZ_GetEventInfo(handle, buffer, size, ev, &eventInfo, &EventPtr);
      if (ret)
	std::cout << "Event Info Readout failed" << std::endl; 
      evtn++; 

      ret = CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event742);
      if (ret)
	std::cout << "Event Info Readout failed" << std::endl;

      CAEN_DGTZ_X742_GROUP_t group = Event742->DataGroup[0];
      for (int i = 0; i < 9; i++) {
	for (int j = 0; j < 1024; j++) {
	  ys[i][j] = group.DataChannel[i][j];
	}
      }

      int minb = TMath::LocMin(1024, ys[8].data());
      minbin = minb; 
      
      tree.Fill();
    }


    std::cout << std::dec << "Group 1 Present:" << (int) Event742->GrPresent[0] << std::endl;
    std::cout << std::dec << "Group 2 Present:" << (int) Event742->GrPresent[1] << std::endl; 
    if (Event742->GrPresent[0] == 1) {
      std::cout << "Group 1 Data Present" << std::endl;
      CAEN_DGTZ_X742_GROUP_t group = Event742->DataGroup[0];
      //std::ofstream fs;
      //fs.open("output.dat", std::fstream::out | std::fstream::binary | std::fstream::trunc); 
      //fs.write(buffer, size); 
      //fs.close(); 
      
      
    }
    if (Event742->GrPresent[1] == 1) { 
      std::cout << "Group 2 Data Present" << std::endl;
    }
    if (Event742->GrPresent[2] == 1) { 
      std::cout << "Group 3 Data Present" << std::endl;
    }
    if (Event742->GrPresent[3] == 1) { 
      std::cout << "Group 4 Data Present" << std::endl;
    }

    
    }
  }
  f.Write(); 
  
  //reset before we're done
  ret = CAEN_DGTZ_Reset(handle);
  std::this_thread::sleep_for(std::chrono::milliseconds(500)); 

  ret = CAEN_DGTZ_CloseDigitizer(handle);
  
  
  
  return 0; 
}
