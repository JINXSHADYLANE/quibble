package com.quibble.dgreed;

import android.app.*;
import android.media.*;
import android.util.Log;

public class Sound {
	public Sound() {
	}

	public void Init() {
		Log.v("dgreed sound", "init");
	}

	public void Close() {
		Log.v("dgreed sound", "close");
	}

	public void Update() {
	}

	public IPlayable LoadSample(String filename) {
		Log.v("dgreed sound", "load sample " + filename);
		return null;
	}

	public IPlayable LoadStream(String filename) {
		Log.v("dgreed sound", "load stream " + filename);
		return null;
	}
}

interface ISource {
	public void Pause();
	public void Resume();
	public void Stop();
	public void SetVolume(float vol);
	public float GetVolume();
	public void SetPos(float pos);
	public float GetPos();
}

interface IPlayable {
	public void Free();
	public void SetVolume(float vol);
	public float GetVolume();
	public float Length();
	public void Play();
	public ISource Play(boolean loop);
	public void Stop();
}

