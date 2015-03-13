package com.mapswithme.maps.location;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.Location;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Build;
import android.os.SystemClock;

import com.mapswithme.util.Constants;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.StubLogger;
import com.mapswithme.util.statistics.Statistics;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.List;

public class WifiLocationScanner extends BroadcastReceiver
{
  private Logger mLogger = StubLogger.get();//SimpleLogger.get(this.toString());

  /// Limit received WiFi accuracy with 20 meters.
  private static final double MIN_PASSED_ACCURACY_M = 20;

  public interface Listener
  {
    public void onWifiLocationUpdated(Location l);

    public Location getLastGpsLocation();
  }

  private Listener mObserver = null;

  private WifiManager mWifi = null;

  /// @return true if was started successfully.
  public boolean startScan(Context context, Listener l)
  {
    mObserver = l;
    if (mWifi == null)
    {
      mWifi = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
      context.registerReceiver(this, new IntentFilter(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION));
      if (mWifi.startScan())
      {
        mLogger.d("WiFi scan is started.");
        return true;
      }
      else
      {
        // onReceive() will never be called on fail
        context.unregisterReceiver(this);
        mWifi = null;
        return false;
      }
    }

    // Already in progress
    mLogger.d("WiFi scan is already in progress.");
    return true;
  }

  public void stopScan(Context context)
  {
    mLogger.d("WiFi scan is stopped.");
    context.unregisterReceiver(this);
    mWifi = null;
  }

  @SuppressLint("NewApi")
  private static void appendId(StringBuilder json)
  {
    json.append(",\"id\":{\"currentTime\":");
    json.append(String.valueOf(System.currentTimeMillis()));

    if (Utils.apiEqualOrGreaterThan(Build.VERSION_CODES.JELLY_BEAN_MR1))
    {
      json.append(",\"elapsedRealtimeNanos\":");
      json.append(String.valueOf(SystemClock.elapsedRealtimeNanos()));
    }

    json.append("}");
  }

  @SuppressLint("NewApi")
  private static void setLocationCurrentTime(JSONObject jID, Location l) throws JSONException
  {
    l.setTime(jID.getLong("currentTime"));

    if (Utils.apiEqualOrGreaterThan(Build.VERSION_CODES.JELLY_BEAN_MR1))
      l.setElapsedRealtimeNanos(jID.getLong("elapsedRealtimeNanos"));
  }

  @Override
  public void onReceive(Context context, Intent intent)
  {
    // Reproduce on Kindle Fire that onReceive is called after unregisterReceiver.
    if (mWifi == null)
      return;

    // Prepare JSON request with BSSIDs
    final StringBuilder json = new StringBuilder("{\"version\":\"2.0\"");
    appendId(json);

    final boolean statsEnabled = Statistics.INSTANCE.isStatisticsEnabled();

    boolean wifiHeaderAdded = false;
    List<ScanResult> results = mWifi.getScanResults();
    for (ScanResult r : results)
    {
      if (r.BSSID != null)
      {
        if (!wifiHeaderAdded)
        {
          json.append(",\"wifi\":[");
          wifiHeaderAdded = true;
        }
        json.append("{\"mac\":\"");
        json.append(r.BSSID);
        json.append("\",\"ss\":\"");
        json.append(String.valueOf(r.level));
        if (statsEnabled)
        {
          json.append("\",\"ssid\":\"");
          json.append(r.SSID == null ? " " : r.SSID);
          json.append("\",\"freq\":");
          json.append(String.valueOf(r.frequency));
          json.append(",\"caps\":\"");
          json.append(r.capabilities);
        }
        json.append("\"},");
      }
    }
    if (wifiHeaderAdded)
    {
      json.deleteCharAt(json.length() - 1);
      json.append("]");
    }

    if (statsEnabled)
    {
      final Location l = mObserver.getLastGpsLocation();
      if (l != null)
      {
        if (wifiHeaderAdded)
          json.append(",");
        json.append("\"gps\":{\"latitude\":");
        json.append(String.valueOf(l.getLatitude()));
        json.append(",\"longitude\":");
        json.append(String.valueOf(l.getLongitude()));
        if (l.hasAccuracy())
        {
          json.append(",\"accuracy\":");
          json.append(String.valueOf(l.getAccuracy()));
        }
        if (l.hasAltitude())
        {
          json.append(",\"altitude\":");
          json.append(String.valueOf(l.getAltitude()));
        }
        if (l.hasSpeed())
        {
          json.append(",\"speed\":");
          json.append(String.valueOf(l.getSpeed()));
        }
        if (l.hasBearing())
        {
          json.append(",\"bearing\":");
          json.append(String.valueOf(l.getBearing()));
        }
        json.append(",\"time\":");
        json.append(String.valueOf(l.getTime()));
        json.append("}");
      }
    }
    json.append("}");

    final String jsonString = json.toString();

    new AsyncTask<String, Void, Boolean>()
    {
      // Result for Listener
      private Location mLocation = null;

      @Override
      protected void onPostExecute(Boolean result)
      {
        // Notify event should be called on UI thread
        if (mObserver != null && mLocation != null)
          mObserver.onWifiLocationUpdated(mLocation);
      }

      @Override
      protected Boolean doInBackground(String... params)
      {
        // Send http POST to location service
        HttpURLConnection conn = null;
        OutputStreamWriter wr = null;
        BufferedReader rd = null;

        try
        {
          final URL url = new URL(Constants.Url.GEOLOCATION_SERVER_MAPSME);
          conn = (HttpURLConnection) url.openConnection();
          conn.setUseCaches(false);

          // Write JSON query
          mLogger.d("Post JSON request with length = ", jsonString.length());
          conn.setDoOutput(true);

          wr = new OutputStreamWriter(conn.getOutputStream());
          wr.write(jsonString);
          wr.flush();
          Utils.closeStream(wr);

          // Get the response
          mLogger.d("Get JSON response with code = ", conn.getResponseCode());
          rd = new BufferedReader(new InputStreamReader(conn.getInputStream(), "UTF-8"));
          String line = null;
          String response = "";
          while ((line = rd.readLine()) != null)
            response += line;

          final JSONObject jRoot = new JSONObject(response);
          final JSONObject jLocation = jRoot.getJSONObject("location");
          final double lat = jLocation.getDouble("latitude");
          final double lon = jLocation.getDouble("longitude");
          final double acc = jLocation.getDouble("accuracy");

          mLocation = new Location("wifiscanner");
          mLocation.setAccuracy((float) Math.max(MIN_PASSED_ACCURACY_M, acc));
          mLocation.setLatitude(lat);
          mLocation.setLongitude(lon);
          setLocationCurrentTime(jRoot.getJSONObject("id"), mLocation);

          return true;
        } catch (IOException e)
        {
          mLogger.d("Unable to get location from server: ", e);
        } catch (JSONException e)
        {
          mLogger.d("Unable to parse JSON responce: ", e);
        } finally
        {
          if (conn != null)
            conn.disconnect();

          Utils.closeStream(wr);
          Utils.closeStream(rd);
        }

        return false;
      }

    }.execute(jsonString);
  }
}