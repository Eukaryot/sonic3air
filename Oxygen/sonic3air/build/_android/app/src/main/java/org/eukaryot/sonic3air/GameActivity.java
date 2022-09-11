package org.eukaryot.sonic3air;

import android.content.ContentResolver;
import android.content.Intent;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;


// TODO: Create a new package in Oxygen (not S3AIR) with a base class for the more general declarations and functions
public class GameActivity extends SDLActivity
{
    // Static native methods
    public static native void receivedRomContent(boolean success, byte[] data);

    // Active instance of this class, needed for communication with IntentReceiverActivity
    public static GameActivity instance = null;

    private static byte[] mRomContent = null;

    // Meant to be called by IntentReceiverActivity
    public static boolean openRomUri(Uri uri, ContentResolver contentResolver)
    {
        Log.i("oxygen", "ROM path from intent = " + uri.getPath());
        byte[] byteArray = readRomUri(uri, contentResolver);
        if (null != byteArray)
        {
            receivedRomContent(true, byteArray);
            return true;
        }
        return false;
    }

    public static byte[] readRomUri(Uri uri, ContentResolver contentResolver)
    {
        try
        {
            // Read file content into a byte array
            InputStream inputStream = contentResolver.openInputStream(uri);
            ByteArrayOutputStream buffer = new ByteArrayOutputStream();
            {
                int bytesWritten = 0;
                int bytesRead = 0;
                byte[] data = new byte[1024];
                while ((bytesRead = inputStream.read(data, 0, data.length)) != -1)
                {
                    buffer.write(data, 0, bytesRead);
                    bytesWritten += bytesRead;
                    if (bytesWritten > 1024 * 1024 * 4)
                    {
                        Log.e("oxygen", "ROM file is too large");
                        return null;
                    }
                }
                buffer.flush();
            }
            return buffer.toByteArray();
        }
        catch (FileNotFoundException e)
        {
            e.printStackTrace();
        }
        catch (IOException e)
        {
            e.printStackTrace();
        }

        // Error
        return null;
    }

    public boolean hasRomFileAlready()
    {
        if (null != mRomContent)
        {
            receivedRomContent(true, mRomContent);
            return true;
        }
        return false;
    }

    public void openRomFileSelectionDialog()
    {
        // Don't actually open the file dialog if we got the ROM content already
        if (null != mRomContent)
        {
            receivedRomContent(true, mRomContent);
            mRomContent = null;     // Ready to reset now
        }
        else
        {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("application/octet-stream");
            startActivityForResult(intent, 0x0000feed);
        }
    }

    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        // Set as instance, so that it can be queried by IntentReceiverActivity
        instance = this;

        // Create the external files directory (and mods inside), and make sure it shows up when accessing the directory structure from a PC
        File path = new File(getExternalFilesDir(null).getAbsoluteFile(), "mods");   // Using just any file name that will be created anyways
        if (!path.exists())
        {
            Log.i("oxygen", "Creating files and mods directory: " + path.getAbsolutePath());
            path.mkdirs();

            // Performing a media scan is needed
            MediaScannerConnection.scanFile(this, new String[] { path.toString() }, null, null);
        }

        if (null != getIntent().getType())
        {
            Log.i("oxygen", "Create: Got an intent: type = " + getIntent().getType() + ", data = " + getIntent().getData());

            // Check the intent, in case this activity was created by the IntentReceiverActivity
            if (getIntent().getType().equals("application/octet-stream"))
            {
                mRomContent = readRomUri(getIntent().getData(), getContentResolver());
            }
        }
    }

    @Override
    protected void onResume()
    {
        super.onResume();
    }

    @Override
    protected void onDestroy()
    {
        instance = null;
        super.onDestroy();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent)
    {
        super.onActivityResult(requestCode, resultCode, intent);
        Log.i("oxygen", "Got activity result, request code = " + requestCode + ", result code = " + resultCode);

        if (requestCode == 0x0000feed)
        {
            if (null != intent)
            {
                // File selection returned a result
                Uri uri = intent.getData();
                Log.i("oxygen", "ROM path from intent = " + uri.getPath());
                mRomContent = readRomUri(uri, getContentResolver());
                receivedRomContent(null != mRomContent, mRomContent);
            }
            else
            {
                // File selection got aborted
                receivedRomContent(false, null);
            }
        }
    }
}
