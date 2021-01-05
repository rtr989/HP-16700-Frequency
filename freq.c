/*****************************************************************************
 * Frequency meter for HP 16700 series logic analyzers.
 * Andrei A. Semenov <rtr989@gmail.com>
 *
 * See the LICENSE file accompying this source file for copyright and
 * redistribution information. Copyright (c) 2020 by Andrei A. Semenov.
 *
 * For use with the Tool Development Kit.
 *****************************************************************************/

void execute(TDKDataGroup &dg, TDKBaseIO &io) {
  int err = 0;

  TDKDataSet ds;
  TDKDataSet dataDs;
  TDKDataSet eventDs;

  TDKLabelEntry le;
  unsigned int freqValue;

  TDKLabelEntry freqLE;
  TDKLabelEntry EventsLE;

  long long correlationTime;
  unsigned int origNumSamples;
  int triggerRow;
  long long time, startTime, stopTime;
  float byTime;

  String analyzer = io.getArg(0);
  String channel = io.getArg(1);

  err = ds.attach(dg, analyzer);
  if (err) {
    io.printError(err);
    return;
  }

  err = le.attach(ds, channel);
  if (err) {
    io.printError(err);
    return;
  }

  correlationTime = ds.getCorrelationTime();
  ds.setTimeBias();

  origNumSamples = ds.getNumberOfSamples();

  ds.peekNext(time);
  triggerRow = -1;

  while (time <= nanoSec(0)) {
    ds.next(time);
    triggerRow++;
  }
  triggerRow = 0;

  ds.reset();

  err = eventDs.createTimeTags(dg, "Events", origNumSamples, triggerRow,
                               correlationTime, nanoSec(0.5));
  if (err) {
    io.printError(err);
    return;
  }
  eventDs.setTimeBias();
  eventDs.reset();
  eventDs.displayStateNumberLabel(false);

  err = EventsLE.createTextData(eventDs, "Frequency", 32);
  if (err) {
    io.printError(err);
    return;
  }

  int cnt = 0;
  int prevFreqVal = 0;

  ds.setPosition(ds.firstPosition());
  le.setPosition(le.firstPosition());

  while (ds.next(time) && le.next(freqValue)) {
    if (freqValue == 1 && prevFreqVal == 0) {
      if (cnt == 1) {
        startTime = time;
      }

      cnt++;
      stopTime = time;
    }
    prevFreqVal = freqValue;
  }
  if (startTime != 0 && stopTime != 0 && cnt != 0) {
    long long tmp = (stopTime - startTime) / (cnt - 2);
    byTime = 1000000000.0 / tmp;
  }

  io.printf("Total states: %d", cnt);

  long long *times = new long long[cnt];
  long long *periodTimes = new long long[cnt - 1];

  ds.setPosition(ds.firstPosition());
  le.setPosition(le.firstPosition());

  cnt = 0;
  prevFreqVal = 0;
  while (ds.next(time) && le.next(freqValue)) {
    if (freqValue == 1 && prevFreqVal == 0) {

      times[cnt] = time;
      cnt++;
    }
    prevFreqVal = freqValue;
  }

  for (int a = 0; a < cnt - 1; a++) {
    periodTimes[a] = times[a + 1] - times[a];
  }

  int periodCounter = 0;
  long long finalTimes[10];
  // float resFreq[10];
  float byState;
  long long finalCounter[10];
  int inc = 0;
  bool write = true;

  for (int c = 0; c < 10; c++) {
    finalCounter[c] = 0;
    finalTimes[c] = 0;
  }

  for (int i = 0; i < cnt - 1; i++) {
    for (int a = 0; a < cnt - 1; a++) {
      if (periodTimes[i] == periodTimes[a]) {
        periodCounter++;
      }
    }

    if (periodCounter >= cnt / 15) {

      for (int c = 0; c < 10; c++) {
        if (periodTimes[i] == finalTimes[c]) {
          write = false;
        }
      }
      if (write) {
        finalCounter[inc] = periodCounter;
        finalTimes[inc] = periodTimes[i];
        inc++;
      }
    }
    write = true;
    periodCounter = 0;
  }

  long long per = 0;
  int sum = 0;

  for (int cc = 0; cc < 10; cc++) {
    if (finalCounter[cc] > 0) {
      per += finalTimes[cc];
      sum++;
    }
  }

  if (sum != 0 && per != 0) {
    byState = 1000000000.0 / (per / sum);
  }

  eventDs.setPosition(0);
  EventsLE.setPosition(0);

  char freqByTime[200];
  sprintf(freqByTime, "Frequency by time:  %.3f kHz", byTime);
  eventDs.replaceNext(0);
  EventsLE.setColor(EventsLE.getPosition(), 1);
  EventsLE.replaceNext((String)freqByTime);
  io.printf(freqByTime);

  char freqByState[200];
  sprintf(freqByState, "Frequency by state: %.3f kHz", byState);
  eventDs.replaceNext(1);
  EventsLE.setColor(EventsLE.getPosition(), 2);
  EventsLE.replaceNext((String)freqByState);
  io.printf(freqByState);
}

StringList getLabelNames() {
  StringList labels;
  labels.put("Analyzer name: ");
  labels.put("Channel name: ");
  return labels;
}

// Assign default values to runtime arguments
StringList getDefaultArgs() {
  StringList defs;
  defs.put("Analyzer<E>_TZ");
  defs.put("FREQ_TZ");
  return defs;
}
