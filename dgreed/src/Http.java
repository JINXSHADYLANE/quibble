package com.quibble.dgreed;

import com.loopj.android.http.*;
import android.util.Log;
import org.apache.http.entity.StringEntity;

public class Http {
	private static AsyncHttpClient client = new AsyncHttpClient();

	public static native void nativeCallback(int callbackId, int retcode, String data, String header);

	public static void get(String addr, boolean getHeader, int callbackId) {
		final int id = callbackId;
		client.get(addr, new AsyncHttpResponseHandler() {
			@Override
			public void onSuccess(int statusCode, String response) {
				nativeCallback(id, statusCode, response, null);
			}

			@Override
			public void onFailure(Throwable error) {
				nativeCallback(id, 0, null, null);
			}
		});
	}

	public static void post(String addr, boolean getHeader, String data, String contentType, int callbackId) {
		final int id = callbackId;
		StringEntity payload = null;

		try {
			payload = new StringEntity(data);
		}
		catch(Exception e) {
		}

		client.post(null, addr, payload, contentType, new AsyncHttpResponseHandler() {
			@Override
			public void onSuccess(int statusCode, String response) {
				nativeCallback(id, statusCode, response, null);
			}

			@Override
			public void onFailure(Throwable error) {
				nativeCallback(id, 0, null, null);
			}
		});
	}

	public static void put(String addr, boolean getHeader, String data, String contentType, int callbackId) {
		final int id = callbackId;
		StringEntity payload = null;

		try {
			payload = new StringEntity(data);
		}
		catch(Exception e) {
		}

		client.put(null, addr, payload, contentType, new AsyncHttpResponseHandler() {
			@Override
			public void onSuccess(int statusCode, String response) {
				nativeCallback(id, statusCode, response, null);
			}

			@Override
			public void onFailure(Throwable error) {
				nativeCallback(id, 0, null, null);
			}
		});
	}

	public static void delete(String addr, boolean getHeader, int callbackId) {
		final int id = callbackId;
		client.delete(addr, new AsyncHttpResponseHandler() {
			@Override
			public void onSuccess(int statusCode, String response) {
				nativeCallback(id, statusCode, response, null);
			}

			@Override
			public void onFailure(Throwable error) {
				nativeCallback(id, 0, null, null);
			}
		});
	}
}

