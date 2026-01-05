package org.eukaryot.sonic3air;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;

public class IntentReceiverActivity extends Activity
{
	public void onGameActivityReady()
	{
		Uri uri = getIntent().getData();
		GameActivity.openRomUri(uri, getContentResolver());
	}

	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		Log.i("oxygen", "IntentReceiverActivity create with intent: Type = " + getIntent().getType() + ", Data = " + getIntent().getData());

		if (null == GameActivity.instance)
		{
			// Start the game activity
			try
			{
				Class<?> c = Class.forName("org.eukaryot.sonic3air.GameActivity");
				Intent intent = new Intent(this, c);
				intent.setDataAndType(getIntent().getData(), getIntent().getType());
				startActivity(intent);
			}
			catch (ClassNotFoundException ignored)
			{
			}
			finish();
		}
		else
		{
			onGameActivityReady();

			AlertDialog.Builder dlgAlert  = new AlertDialog.Builder(this);
			dlgAlert.setTitle("Sonic 3 A.I.R. file handler");
			dlgAlert.setMessage("Passed file to Sonic 3 A.I.R.\nPress OK to switch back to the game.");
			dlgAlert.setPositiveButton("Ok",
				new DialogInterface.OnClickListener()
				{
					public void onClick(DialogInterface dialog, int which)
					{
						String activityToStart = "org.eukaryot.sonic3air.GameActivity";
						try
						{
							Class<?> c = Class.forName(activityToStart);
							Intent intent = new Intent(IntentReceiverActivity.this, c);
							startActivity(intent);
						}
						catch (ClassNotFoundException ignored)
						{
						}
						finish();
					}
				});
			dlgAlert.setCancelable(true);
			dlgAlert.create().show();
		}
	}
}
