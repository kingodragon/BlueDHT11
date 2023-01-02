int32_t channel_1_start=0, channel_1_stop=0, channel_1=0;
uint8_t DHT11=PA0;
uint8_t I_RH,D_RH,I_Temp,D_Temp,CheckSum; 
uint8_t DHT11_data[50]={0},index1=0;
char temp[30]="",hum[30]="";
void setup() {
 // systick_disable();
   Serial2.begin(115200);
   Serial2.println(F("DHTxx test!"));
   pinMode(PB11, OUTPUT);
   print_reg();
}

void loop() {
  // put your main code here, to run repeatedly:
    Timer2.detachInterrupt(TIMER_CH1);
    Request() ;  /* send start pulse */
    delay(200);
    if(index1==41&&((I_RH + D_RH + I_Temp + D_Temp) == CheckSum))
    {
      Receive_data();
      sprintf(temp, "%s%d%c%d %s" , "    ",I_Temp,'.' ,D_Temp,"C"); 
      sprintf(hum, "%s%d%c%d %c    " , "    ",I_RH,'.',D_RH,'%'); 
      Serial2.println(temp);
      Serial2.println(hum);
      Serial2.println("OK");
    }
    else
    {
      Serial2.println("error");
    }
    for(int i=0;i<50;i++)
      DHT11_data[i]=0;
    delay(2000);
    index1=0;
}


void Request()      /* Microcontroller send request */
{
  pinMode(DHT11, OUTPUT);
 // print_reg();
  digitalWrite(DHT11,LOW); 
  uint16_t PulseLOW = 25000; // should > 18ms
  uint16_t PulseHIGH = 30; // not important
  /*Timer2.detachInterrupt(TIMER_CH1);
  Timer2.pause(); // stop the timers before configuring them
  Timer2.setChannel1Mode(TIMER_OUTPUT_COMPARE);
 // timer_oc_set_mode(TIMER2, 1, TIMER_OC_MODE_PWM_2, TIMER_OC_PE);//set PWM Mode2,and enable
  Timer2.setPrescaleFactor(72); // 1 microsecond resolution
  Timer2.setOverflow(PulseLOW + PulseHIGH-1);
  Timer2.setCompare(TIMER_CH1, PulseLOW);
 // Triggered after PulseLOW
  Timer2.attachInterrupt(TIMER_CH1,Response);  
  Timer2.refresh(); // start timer 2
  Timer2.resume(); // let timer 2 run
*/
  pinMode(DHT11, PWM);
  Timer2.pause(); // stop the timers before configuring them
  timer_oc_set_mode(TIMER2, 1, TIMER_OC_MODE_PWM_2, TIMER_OC_PE);//set PWM Mode2,and enable
  Timer2.setPrescaleFactor(72); // 1 microsecond resolution
  Timer2.setOverflow(PulseLOW + PulseHIGH-1);
  Timer2.setCompare(TIMER_CH1, PulseLOW);
 // Triggered after PulseLOW
  Timer2.attachInterrupt(TIMER_CH1,Response);
  // counter setup in one pulse mode
  //TIMER2_BASE->CR1 = TIMER_CR1_CEN;
  TIMER2_BASE->CR1 = ( TIMER_CR1_OPM ); // one pulse mode
  TIMER2_BASE->CCER =  TIMER_CCER_CC1E ; // enable channels 1 
  Timer2.refresh(); // start timer 2
  Timer2.resume(); // let timer 2 run
  
/*
                    Using PWM Mode 2

  _______20MS__________-------------------18MS-------------------___________
  ^                    ^                                         ^
  |<----PulseLOW------>|<-- PulseHIGH, but don't need attention->|      
                       ^
                       |
                       
              Compare1Interrupt
              execution Response(),
           Change the mode of the Timer2

 */

}
 

void Response()
{
    pinMode(DHT11, INPUT_PULLUP);
    Timer2.pause(); 
    Timer2.attachInterrupt(TIMER_CH1,handler_channel_1);
    TIMER2_BASE->CCR1=0;
    TIMER2_BASE->CR1 = TIMER_CR1_CEN;
    TIMER2_BASE->CR2 = 0;
    TIMER2_BASE->SMCR = 0;
    TIMER2_BASE->DIER = TIMER_DIER_CC1IE ;
    TIMER2_BASE->EGR = 0;
    TIMER2_BASE->CCMR1 = 0b100000001; //Register is set like this due to a bug in the define table (30-09-2017)
    TIMER2_BASE->CCMR2 = 0;
    TIMER2_BASE->CCER = TIMER_CCER_CC1E ;
    TIMER2_BASE->PSC = 71;
    TIMER2_BASE->ARR = 0xFFFF;
}

void print_reg()
{
      Serial2.println("TIMER2_BASE->CR1 ="+ (String)TIMER2_BASE->CR1+';');
      Serial2.println("TIMER2_BASE->CR2 =" +(String)TIMER2_BASE->CR2+';');
      Serial2.println("TIMER2_BASE->SMCR ="+ (String)TIMER2_BASE->SMCR+';');
      Serial2.println("TIMER2_BASE->DIER ="+ (String)TIMER2_BASE->DIER+';') ;
      Serial2.println("TIMER2_BASE->EGR = "+(String)TIMER2_BASE->EGR+';');
      Serial2.println("TIMER2_BASE->CCMR1 = "+(String)TIMER2_BASE->CCMR1+';');
      Serial2.println("TIMER2_BASE->CCMR2 = "+(String)TIMER2_BASE->CCMR2+';');
      Serial2.println("TIMER2_BASE->CCER = "+(String)TIMER2_BASE->CCER+';') ;
      Serial2.println("TIMER2_BASE->PSC = "+(String)TIMER2_BASE->PSC+';');
      Serial2.println("TIMER2_BASE->ARR = "+(String)TIMER2_BASE->ARR+';');
      Serial2.println("TIMER2_BASE->DCR = "+(String)TIMER2_BASE->DCR+';');
      Serial2.println();
}

void handler_channel_1(void) {                           //This function is called when channel 1 is captured.
    if (0b1 & GPIOA_BASE->IDR  >> 0) {                     //If the receiver channel 1 input pulse on A0 is high.
      channel_1_start = TIMER2_BASE->CCR1;                 //Record the start time of the pulse.
      TIMER2_BASE->CCER |= TIMER_CCER_CC1P;                //Change the input capture mode to the falling edge of the pulse.
    }
    else {                                                 //If the receiver channel 1 input pulse on A0 is low.
      channel_1 = TIMER2_BASE->CCR1 - channel_1_start;     //Calculate the total pulse time.
      if (channel_1 < 0)channel_1 += 0xFFFF;               //If the timer has rolled over a correction is needed.
      TIMER2_BASE->CCER &= ~TIMER_CCER_CC1P;               //Change the input capture mode to the rising edge of the pulse.
      DHT11_data[index1++]=channel_1;
      if(index1==41)
        Timer2.pause(); 
    }
}
void Receive_data()    /* Receive data */
{
    int q=0;  
    I_RH=0;
    D_RH=0;
    I_Temp=0;
    D_Temp=0;
    CheckSum=0;
    for (q=1; q<9; q++)
    {
      if(DHT11_data[q]>30)  /* If high pulse is greater than 30ms */
      I_RH = (I_RH<<1)|(0x01);/* Then its logic HIGH */
      else    /* otherwise its logic LOW */
      I_RH = (I_RH<<1);
    }     
    for (q=9; q<17; q++)
    {
      if(DHT11_data[q]>30)  /* If high pulse is greater than 30ms */
      D_RH = (D_RH<<1)|(0x01);/* Then its logic HIGH */
      else    /* otherwise its logic LOW */
      D_RH = (D_RH<<1);
    }   
    for (q=17; q<25; q++)
    {
      if(DHT11_data[q]>30)  /* If high pulse is greater than 30ms */
      I_Temp = (I_Temp<<1)|(0x01);/* Then its logic HIGH */
      else    /* otherwise its logic LOW */
      I_Temp = (I_Temp<<1);
    }
      for (q=25; q<33; q++)
    {
      if(DHT11_data[q]>30)  /* If high pulse is greater than 30ms */
      D_Temp = (D_Temp<<1)|(0x01);/* Then its logic HIGH */
      else    /* otherwise its logic LOW */
      D_Temp = (D_Temp<<1);
    } 
    for (q=33; q<41; q++)
    {
      if(DHT11_data[q]>30)  /* If high pulse is greater than 30ms */
      CheckSum = (CheckSum<<1)|(0x01);/* Then its logic HIGH */
      else    /* otherwise its logic LOW */
      CheckSum = (CheckSum<<1);
    }                                                                                                                                                                       
}
