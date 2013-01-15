package com.quibble.dgreed;

import java.io.IOException;
import org.libsdl.app.SDLActivity;
import android.content.Context;
import android.content.res.*;
import android.app.*;
import android.media.SoundPool;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.util.Log;

public class Sound {
	private SoundPool pool;

	public Sound() {
	}

	public void Init() {
		pool = new SoundPool(4, AudioManager.STREAM_MUSIC, 0);
	}

	public void Close() {
		pool.release();
		pool = null;
	}

	public void Update() {
	}

	public IPlayable LoadSample(String filename) {
		return new SoundPoolSound(pool, filename);
	}

	public IPlayable LoadStream(String filename) {
		return new MediaPlayerSound(filename);
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

class MediaPlayerSound implements IPlayable, ISource {
	private MediaPlayer player;	
	private float volume;

	public MediaPlayerSound(String filename) {
		Context ctx = SDLActivity.getContext();
		AssetManager am = ctx.getAssets();

		try {
			AssetFileDescriptor fd = am.openFd(filename);	
			player = new MediaPlayer();
            player.setDataSource(fd.getFileDescriptor(),fd.getStartOffset(),fd.getLength());
            fd.close();
            player.setLooping(true);
            player.prepare();
		}
		catch(IOException e) {
			Log.v("dgreedJ", "error playing sound stream");
		}

		volume = 1.0f;
	}

	public void Free() {
		player.reset();
	}

	public void SetVolume(float vol) {
		player.setVolume(vol, vol);
		volume = vol;
	}

	public float GetVolume() {
		return volume;
	}

	public float Length() {
		return (float)player.getDuration() / 1000.0f;
	}

	public void Play() {
		if(!player.isPlaying()) {
			player.start();	
		}
	}

	public ISource Play(boolean loop) {
		player.setLooping(loop);
		if(!player.isPlaying()) {
			player.start();	
		}
		return this;
	}

	public void Stop() {
		if(player.isPlaying()) {
			player.stop();
		}
	}

	public void Pause() {
		player.pause();
	}

	public void Resume() {
		player.start();
	}

	public void SetPos(float pos) {
		player.seekTo((int)(pos * 1000.0f));
	}

	public float GetPos() {
		return (float)player.getCurrentPosition() / 1000.0f;
	}
}

class SoundPoolSource implements ISource {
	private SoundPool pool;
	private int id;
	private float volume;

	public SoundPoolSource(SoundPool owner, int streamId, float vol) {
		pool = owner;
		id = streamId;
		volume = vol;

		if(vol != 1.0f) {
			pool.setVolume(id, vol, vol);
		}
	}

	public void Pause() {
		if(id != -1) {
			pool.pause(id);
		}
	}

	public void Resume() {
		if(id != -1) {
			pool.resume(id);
		}
	}

	public void Stop() {
		if(id != -1) {
			pool.stop(id);
			id = -1;
		}
	}

	public void SetVolume(float vol) {
		if(id != -1) {
			pool.setVolume(id, vol, vol);
			volume = vol;
		}
	}

	public float GetVolume() {
		return volume;
	}

	public void SetPos(float pos) {
		// Not implemented
	}

	public float GetPos() {
		return 0.0f;
	}
}

class SoundPoolSound implements IPlayable {
	private SoundPool pool;
	private int id;
	private float volume;
	private int lastStream;

	public SoundPoolSound(SoundPool owner, String filename) {
		Context ctx = SDLActivity.getContext();
		AssetManager am = ctx.getAssets();
		pool = owner;

		try {
			Log.v("dgreedJ", "Loading sound " + filename);
			AssetFileDescriptor fd = am.openFd(filename);	
			id = pool.load(fd, 1);
		}
		catch(IOException e) {
			id = -1;
		}

		volume = 1.0f;
		lastStream = -1;
	}

	public void Free() {
		if(id != -1) {
			pool.unload(id);
			id = -1;
		}
	}

	public void SetVolume(float vol) {
		volume = vol;
	}

	public float GetVolume() {
		return volume;
	}

	public float Length() {
		// Not implemented
		return 0.0f;
	}

	public void Play() {
		if(id != -1) {
			int priority = 0;
			int loop = 0;
			lastStream = pool.play(id, volume, volume, priority, loop, 1.0f);
		}
	}

	public ISource Play(boolean loop) {
		return null;
	}

	public void Stop() {
		if(lastStream != -1) {
			pool.stop(lastStream);
			lastStream = -1;
		}
	}
}

