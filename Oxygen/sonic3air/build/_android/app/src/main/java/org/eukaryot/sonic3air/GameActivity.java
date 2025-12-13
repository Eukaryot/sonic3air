package org.eukaryot.sonic3air;

import android.app.DownloadManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;


// TODO: Create a new package in Oxygen (not S3AIR) with a base class for the more general declarations and functions
public class GameActivity extends SDLActivity
{
	// Static native methods
	public static native void receivedRomContent(boolean success, byte[] data);
	public static native void receivedFileContent(boolean success, byte[] data, String path);
	public static native void grantedFolderAccess(boolean success, String path);


	// Active instance of this class, needed for communication with IntentReceiverActivity
	public static GameActivity instance = null;

	private static byte[] mRomContent = null;
	private static byte[] mFileExportContent = null;
	private static String mFileExportName = null;

	private final static int REQUEST_CODE_ROM_FILE_SELECTION = 0xee01;
	private final static int REQUEST_CODE_FILE_SELECTION     = 0xee02;
	private final static int REQUEST_CODE_FILE_EXPORT        = 0xee03;
	private final static int REQUEST_CODE_FOLDER_SELECTION   = 0xee04;


	@Override
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
				mRomContent = readFileContentsFromUri(getIntent().getData(), getContentResolver(), 0x400000);
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

		switch (requestCode)
		{
			case REQUEST_CODE_ROM_FILE_SELECTION:
			{
				if (null != intent && resultCode == RESULT_OK)
				{
					// File selection returned a result
					Uri uri = intent.getData();
					Log.i("oxygen", "ROM path from intent = " + uri.getPath());
					mRomContent = readFileContentsFromUri(uri, getContentResolver(), 0x400000);
					receivedRomContent(null != mRomContent, mRomContent);
				}
				else
				{
					// File selection got aborted
					receivedRomContent(false, null);
				}
				break;
			}

			case REQUEST_CODE_FILE_SELECTION:
			{
				if (null != intent && resultCode == RESULT_OK)
				{
					// File selection returned a result
					Uri uri = intent.getData();
					Log.i("oxygen", "File path from intent = " + uri.getPath());
					byte[] fileContent = readFileContentsFromUri(uri, getContentResolver(), 0x40000000);    // 1 GB file size limit
					receivedFileContent(null != fileContent, fileContent, uri.getPath());
				}
				else
				{
					// File selection got aborted
					receivedFileContent(false, null, "");
				}
				break;
			}

			case REQUEST_CODE_FILE_EXPORT:
			{
				if (null != intent && resultCode == RESULT_OK)
				{
					// Export location selection returned a result
					Uri uri = intent.getData();
					Log.i("oxygen", "Selected export path from intent = " + uri.getPath());

					ContentResolver contentResolver = getContentResolver();
					Uri documentUri = null;
					try
					{
						Uri parentDocumentUri = DocumentsContract.buildDocumentUriUsingTree(uri, DocumentsContract.getTreeDocumentId(uri));
						documentUri = DocumentsContract.createDocument(contentResolver, parentDocumentUri, "application/octet-stream", mFileExportName);
						if (documentUri != null)
						{
							OutputStream out = contentResolver.openOutputStream(documentUri);
							out.write(mFileExportContent);
						}
						else
						{
							Log.i("oxygen", "Failed to write file during export, invalid documentUri");
						}
					}
					catch (Exception e)
					{
						Log.i("oxygen", "Failed to write file during export, with exception: " + e.toString());
					}
				}
				break;
			}

			case REQUEST_CODE_FOLDER_SELECTION:
			{
				if (null != intent && resultCode == RESULT_OK)
				{
					// Folder selection returned a result
					Uri uri = intent.getData();
					Log.i("oxygen", "Selected folder path from intent = " + uri.getPath());
					grantedFolderAccess(true, uri.getPath());
				}
				else
				{
					// Folder selection got aborted
					grantedFolderAccess(false, "");
				}
				break;
			}
		}
	}


	// Meant to be called by IntentReceiverActivity
	public static boolean openRomUri(Uri uri, ContentResolver contentResolver)
	{
		Log.i("oxygen", "ROM path from intent = " + uri.getPath());
		byte[] byteArray = readFileContentsFromUri(uri, contentResolver, 0x400000);
		if (null != byteArray)
		{
			receivedRomContent(true, byteArray);
			return true;
		}
		return false;
	}

	private static byte[] readFileContentsFromUri(Uri uri, ContentResolver contentResolver, int fileSizeLimit)
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
					if (bytesWritten > fileSizeLimit)
					{
						Log.e("oxygen", "File is too large");
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


	// Called from C++
	public boolean hasRomFileAlready()
	{
		if (null != mRomContent)
		{
			receivedRomContent(true, mRomContent);
			return true;
		}
		return false;
	}

	// Called from C++
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
			startActivityForResult(intent, REQUEST_CODE_ROM_FILE_SELECTION);
		}
	}


	// Called from C++
	public void openFileSelectionDialog()
	{
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
		intent.addCategory(Intent.CATEGORY_OPENABLE);
		intent.setType("*/*");
		startActivityForResult(intent, REQUEST_CODE_FILE_SELECTION);
	}


	// Called from C++
	public void openFileExportDialog(String filename, byte[] content)
	{
		mFileExportName = filename;
		mFileExportContent = content;

		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
		intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
		startActivityForResult(intent, REQUEST_CODE_FILE_EXPORT);
	}


	// Called from C++
	public void openFolderAccessDialog()
	{
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
		startActivityForResult(intent, REQUEST_CODE_FOLDER_SELECTION);
	}


	// Called from C++
	public long startFileDownload(String url, String filename)
	{
		Log.i("oxygen", "Starting download from '" + url + "' to file '" + filename + "'");

		Uri uri = Uri.parse(url);
		DownloadManager.Request request = new DownloadManager.Request(uri);
		request.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE);
		request.setAllowedOverMetered(true);
		request.setDestinationUri(Uri.fromFile(new File(getExternalFilesDir(null).getAbsoluteFile(), filename)));
		request.setVisibleInDownloadsUi(false);     // Only needed until API level 29

		DownloadManager downloadManager = (DownloadManager)getSystemService(Context.DOWNLOAD_SERVICE);
		long downloadId = downloadManager.enqueue(request);
		return downloadId;
	}

	// Called from C++
	public boolean stopFileDownload(long downloadId)
	{
		DownloadManager downloadManager = (DownloadManager)getSystemService(Context.DOWNLOAD_SERVICE);
		return (downloadManager.remove(downloadId) > 0);
	}

	// Called from C++
	public int getDownloadStatus(long downloadId)
	{
		// Return value:
		//  0x00 = Invalid download request
		//  0x01 = Pending download
		//  0x02 = Running
		//  0x04 = Paused
		//  0x08 = Finished successfully
		//  0x10 = Failed
		Cursor cursor = getDownloadCursor(downloadId);
		return (null != cursor) ? cursor.getInt(cursor.getColumnIndex(DownloadManager.COLUMN_STATUS)) : 0;
	}

	// Called from C++
	public long getDownloadCurrentBytes(long downloadId)
	{
		Cursor cursor = getDownloadCursor(downloadId);
		return (null != cursor) ? cursor.getLong(cursor.getColumnIndex(DownloadManager.COLUMN_BYTES_DOWNLOADED_SO_FAR)) : 0;
	}

	// Called from C++
	public long getDownloadTotalBytes(long downloadId)
	{
		Cursor cursor = getDownloadCursor(downloadId);
		return (null != cursor) ? cursor.getLong(cursor.getColumnIndex(DownloadManager.COLUMN_TOTAL_SIZE_BYTES)) : 0;
	}

	private Cursor getDownloadCursor(long downloadId)
	{
		DownloadManager downloadManager = (DownloadManager)getSystemService(Context.DOWNLOAD_SERVICE);
		Cursor cursor = downloadManager.query(new DownloadManager.Query().setFilterById(downloadId));
		return (cursor.moveToFirst()) ? cursor : null;
	}
}
