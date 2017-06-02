

best <- function(state, outcome) {

  ## Read outcome data
  ## Clarify NA by na.strings
  data <- read.csv("./outcome-of-care-measures.csv", colClasses="character", na.strings="Not Available")
  
  # Type conversion from char to int
  data[,11] <- as.numeric(data[,11]) # heart attack 
  data[,17] <- as.numeric(data[,17]) # heart failure
  data[,23] <- as.numeric(data[,23]) # pneumonia
  
  vOutcome <- c("heart attack", "heart failure", "pneumonia")
  
  # split into group as state 
  splitState <- split(data, data$State)[[state]]
  
  ## Check that state and outcome are valid
  if(!state %in% data$State)  # whether the state is in data%State
  {
  	stop("invalid state")
  }
  if(!outcome %in% vOutcome) #whether the outcome is in vOutcome
  {
  	stop("invalid outcome")
  }
  
## Return hospital name in that state with lowest 30-day death  	  
  if(outcome == "heart attack")
  {
  	nameHospital <- splitState[
  		splitState$Hospital.30.Day.Death..Mortality..Rates.from.Heart.Attack
  		== min(splitState[,11], na.rm = T), 2]
  }
  else if(outcome == "heart failure")
  {
  	nameHospital <- splitState[
  		splitState$Hospital.30.Day.Death..Mortality..Rates.from.Heart.Failure
  		== min(splitState[,17], na.rm = T), 2]
  }
  else if(outcome == "pneumonia")
  {
  	nameHospital <- splitState[
  		splitState$Hospital.30.Day.Death..Mortality..Rates.from.Pneumonia
  		== min(splitState[,23], na.rm = T), 2]
  }
  else
  {
  	stop("error for finding")
  }
  
  ## Sort and return the value
  	sort(nameHospital)
  	return(nameHospital[1])
 }