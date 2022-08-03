package com.example.myapp;

import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;
import java.net.HttpURLConnection;
import java.net.Socket;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLEncoder;

public class PostBuilder {
    private String ip;
    private int port;
    private String name;
    private String locationLink;

    public PostBuilder(String ip, int port){
        this.ip = ip;
        this.port = port;
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setLocationLink(String locationLink) {
        this.locationLink = locationLink;
    }

    public String getIp(){
        return this.ip;
    }

    public int getPort(){
        return this.port;
    }

    public String getName(){
        return this.name;
    }

    public String getLocationLink(){
        return this.locationLink;
    }

    public void sendRequest(){
        try {
            URL url = new URL("http://" + this.ip + ":" + this.port + "/");
            URLConnection urlConnection = url.openConnection();
            urlConnection.setDoOutput(true);
            urlConnection.setConnectTimeout(15000);
            urlConnection.setReadTimeout(15000);
            OutputStreamWriter out = new OutputStreamWriter(urlConnection.getOutputStream());

            out.write(this.getPostString());
            out.close();

            Log.i("INFO", "Se ha enviado la petición de POST");
            InputStream in = new BufferedInputStream(urlConnection.getInputStream());
            BufferedReader reader = new BufferedReader(new InputStreamReader(in));
            StringBuilder result = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                result.append(line);
            }
            Log.d("INFO", "Respuesta del servidor: " + result.toString());
            in.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private String getPostString() throws UnsupportedEncodingException {
        StringBuilder postString = new StringBuilder(); //name=<nombre>&locationLink=<enlace a la ubicación>
        postString.append(URLEncoder.encode("name", "UTF-8"));
        postString.append("=");
        postString.append(URLEncoder.encode(this.name, "UTF-8"));
        postString.append("&");
        postString.append(URLEncoder.encode("locationLink", "UTF-8"));
        postString.append("=");
        postString.append(URLEncoder.encode(this.locationLink, "UTF-8"));
        return postString.toString();
    }

    @Override
    public String toString() {
        return "PostBuilder{" +
                "ip='" + ip + '\'' +
                ", port=" + port +
                ", name='" + name + '\'' +
                ", locationLink='" + locationLink + '\'' +
                '}';
    }
}
