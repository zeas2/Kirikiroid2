package org.tvp.kirikiri2;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import org.cocos2dx.lib.Cocos2dxActivity;
import org.cocos2dx.lib.Cocos2dxGLSurfaceView;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.database.Cursor;
import android.graphics.PixelFormat;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.storage.StorageManager;
import android.preference.PreferenceManager;
import android.provider.BaseColumns;
import android.provider.MediaStore;
import android.provider.MediaStore.MediaColumns;
import android.provider.Settings.Secure;
import android.support.annotation.NonNull;
import android.support.v4.provider.DocumentFile;
import android.telephony.TelephonyManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

/**
 * Utility class for handling the media store.
 */
abstract class MediaStoreUtil {
    public static Uri getUriFromFile(final String path,Context context) {
        ContentResolver resolver = context.getContentResolver();

        Cursor filecursor = resolver.query(MediaStore.Files.getContentUri("external"),
                new String[] { BaseColumns._ID }, MediaColumns.DATA + " = ?",
                new String[] { path }, MediaColumns.DATE_ADDED + " desc");
        filecursor.moveToFirst();

        if (filecursor.isAfterLast()) {
            filecursor.close();
            ContentValues values = new ContentValues();
            values.put(MediaColumns.DATA, path);
            return resolver.insert(MediaStore.Files.getContentUri("external"), values);
        }
        else {
            int imageId = filecursor.getInt(filecursor.getColumnIndex(BaseColumns._ID));
            Uri uri = MediaStore.Files.getContentUri("external").buildUpon().appendPath(
                    Integer.toString(imageId)).build();
            filecursor.close();
            return uri;
        }
    }

    public static final void addFileToMediaStore(final String path,Context context) {
        Intent mediaScanIntent = new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE);
        File file = new File(path);
        Uri contentUri = Uri.fromFile(file);
        mediaScanIntent.setData(contentUri);
        context.sendBroadcast(mediaScanIntent);
    }

    /**
     * Retrieve a thumbnail of a bitmap from the mediastore.
     *
     * @param path
     *            The path of the image
     * @param maxSize
     *            The maximum size of this bitmap (used for selecting the sample size)
     * @return the thumbnail.
     */

    /**
     * Delete the thumbnail of a bitmap.
     *
     * @param path
     *            The path of the image
     */

}

/* This is a fake invisible editor view that receives the input and defines the
 * pan&scan region
 */
class DummyEdit extends View implements View.OnKeyListener {
    InputConnection ic;

    public DummyEdit(Context context) {
        super(context);
        setFocusableInTouchMode(true);
        setFocusable(true);
        setOnKeyListener(this);
    }

    @Override
    public boolean onCheckIsTextEditor() {
        return true;
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {

        // This handles the hardware keyboard input
        if (event.isPrintingKey()) {
            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                ic.commitText(String.valueOf((char) event.getUnicodeChar()), 1);
            }
            return true;
        }

//        if (event.getAction() == KeyEvent.ACTION_DOWN) {
//        	KR2Activity.nativeKeyAction(keyCode, true);
//            return true;
//        } else if (event.getAction() == KeyEvent.ACTION_UP) {
//        	KR2Activity.nativeKeyAction(keyCode, false);
//            return true;
//        }

        return false;
    }
        
    //
    @Override
    public boolean onKeyPreIme (int keyCode, KeyEvent event) {
        // As seen on StackOverflow: http://stackoverflow.com/questions/7634346/keyboard-hide-event
        // FIXME: Discussion at http://bugzilla.libsdl.org/show_bug.cgi?id=1639
        // FIXME: This is not a 100% effective solution to the problem of detecting if the keyboard is showing or not
        // FIXME: A more effective solution would be to change our Layout from AbsoluteLayout to Relative or Linear
        // FIXME: And determine the keyboard presence doing this: http://stackoverflow.com/questions/2150078/how-to-check-visibility-of-software-keyboard-in-android
        // FIXME: An even more effective way would be if Android provided this out of the box, but where would the fun be in that :)
        if (event.getAction()==KeyEvent.ACTION_UP && keyCode == KeyEvent.KEYCODE_BACK) {
            if (KR2Activity.mTextEdit != null && KR2Activity.mTextEdit.getVisibility() == View.VISIBLE) {
            	KR2Activity.hideTextInput();
            	//KR2Activity.nativeKeyboardFocusLost();
            }
        }
        return super.onKeyPreIme(keyCode, event);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        ic = new SDLInputConnection(this, true);

        outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_EXTRACT_UI
                | 33554432 /* API 11: EditorInfo.IME_FLAG_NO_FULLSCREEN */;

        return ic;
    }
}

class SDLInputConnection extends BaseInputConnection {

    public SDLInputConnection(View targetView, boolean fullEditor) {
        super(targetView, fullEditor);

    }

    @Override
    public boolean sendKeyEvent(KeyEvent event) {
        /*
         * This handles the keycodes from soft keyboard (and IME-translated
         * input from hardkeyboard)
         */
        int keyCode = event.getKeyCode();
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            if (event.isPrintingKey()) {
                commitText(String.valueOf((char) event.getUnicodeChar()), 1);
                KR2Activity.nativeCharInput(keyCode);
            } else if(keyCode == KeyEvent.KEYCODE_DEL) {
            	KR2Activity.nativeKeyAction(keyCode, true);
            }
            return true;
        } else if (event.getAction() == KeyEvent.ACTION_UP) {
        	if(keyCode == KeyEvent.KEYCODE_DEL) {
            	KR2Activity.nativeKeyAction(keyCode, false);
            }
        	//KR2Activity.nativeKeyAction(keyCode, false);
            return true;
        }
        return super.sendKeyEvent(event);
    }

    @Override
    public boolean commitText(CharSequence text, int newCursorPosition) {

    	KR2Activity.nativeCommitText(text.toString(), newCursorPosition);

        return super.commitText(text, newCursorPosition);
    }

    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition) {

        //nativeSetComposingText(text.toString(), newCursorPosition);

        return super.setComposingText(text, newCursorPosition);
    }

    //public native void nativeSetComposingText(String text, int newCursorPosition);

    @Override
    public boolean deleteSurroundingText(int beforeLength, int afterLength) {       
        // Workaround to capture backspace key. Ref: http://stackoverflow.com/questions/14560344/android-backspace-in-webview-baseinputconnection
        if (beforeLength == 1 && afterLength == 0) {
            // backspace
            return super.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DEL))
                && super.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_DEL));
        }

        return super.deleteSurroundingText(beforeLength, afterLength);
    }
}

public class KR2Activity extends Cocos2dxActivity {
	static ActivityManager.MemoryInfo memoryInfo = new ActivityManager.MemoryInfo();
	static ActivityManager mAcitivityManager = null;
	static Debug.MemoryInfo mDbgMemoryInfo = new Debug.MemoryInfo();
	public static void updateMemoryInfo() {
		if(mAcitivityManager == null) {
			mAcitivityManager =(ActivityManager)sInstance.getSystemService(Activity.ACTIVITY_SERVICE);
		}
		mAcitivityManager.getMemoryInfo(memoryInfo);
		Debug.getMemoryInfo(mDbgMemoryInfo);
	}
	
	public static long getAvailMemory() {
		return memoryInfo.availMem;
	}

	public static long getUsedMemory() {
		return mDbgMemoryInfo.getTotalPss(); // in kB
	}
	
	static public String getDeviceId() {
		TelephonyManager mgr = (TelephonyManager)GetInstance().getSystemService(Context.TELEPHONY_SERVICE);
		String DeviceID = mgr.getDeviceId();
		if(DeviceID != null) {
		    return "DevID:" + DeviceID;
		}
		String androidId = Secure.getString(GetInstance().getContentResolver(), Secure.ANDROID_ID);
    	if (null != androidId && androidId.length() > 8 &&
    			(!"9774d56d682e549c".equals(androidId)) ) {
    		return "AndroidID:" + androidId;
    	} else if(null != android.os.Build.SERIAL && android.os.Build.SERIAL.length() > 3) {
    		return "AndroidID:" + android.os.Build.SERIAL;
    	}
		return "";
	}

	static public KR2Activity sInstance;
	static public KR2Activity GetInstance() {return sInstance;}

    static {
		System.loadLibrary("ffmpeg");
	//	System.loadLibrary("game");
    }
    
	@Override
	public void onCreate(Bundle savedInstanceState) {
		sInstance = this;
        Sp = PreferenceManager.getDefaultSharedPreferences(this);
		super.onCreate(savedInstanceState);
		/*
		if (Build.VERSION.SDK_INT>=Build.VERSION_CODES.LOLLIPOP) {
			for(String path : getExtSdCardPaths(this)) {
		        if (!isWritableNormalOrSaf(path)) {
		            guideDialogForLEXA(path);
		        }
			}
		}
		*/
		initDump(this.getFilesDir().getAbsolutePath() + "/dump");
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
		System.exit(0);
	}
	
	@Override
	public void onLowMemory() {
		nativeOnLowMemory();
	}
	
	static class DialogMessage
	{
		public String Title;
		public String Text;
		public String[] Buttons;
		public EditText TextEditor = null;

		public DialogMessage()
		{
		}
		
		public void Init(final String title, final String text, final String[] buttons)
		{
			this.Title = title;
			this.Text = text;
			this.Buttons = buttons;
		}
		
		void onButtonClick(int n) {
			if(TextEditor != null) {
				onMessageBoxText(TextEditor.getText().toString());
			}
        	onMessageBoxOK(n);
		}
		
		public AlertDialog.Builder CreateBuilder() {
		/*	TextView showText = new TextView(sInstance);
			showText.setText(Text);
			if (Build.VERSION.SDK_INT>=Build.VERSION_CODES.HONEYCOMB)
				showText.setTextIsSelectable(true);*/
			AlertDialog.Builder builder = new AlertDialog.Builder(sInstance).
                setTitle(Title).
                setMessage(Text).
                //setView(showText).
				setCancelable(false);
			if(Buttons.length >= 1) {
                builder = builder.setPositiveButton(Buttons[0], new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                    	onButtonClick(0);
                    }
                });
			}
			if(Buttons.length >= 2) {
                builder = builder.setNeutralButton(Buttons[1], new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                    	onButtonClick(1);
                    }
                });
    		}
			if(Buttons.length >= 3) {
                builder = builder.setNegativeButton(Buttons[2], new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                    	onButtonClick(2);
                    }
                });
    		}
			return builder;
		}
		
		public void ShowMessageBox()
		{
			CreateBuilder().create().show();
		}
		
		public void ShowInputBox(final String text) {
			AlertDialog.Builder builder = CreateBuilder();
			TextEditor = new EditText(sInstance);  
			LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
			                     LinearLayout.LayoutParams.MATCH_PARENT,
			                     LinearLayout.LayoutParams.MATCH_PARENT);
			TextEditor.setLayoutParams(lp);
			TextEditor.setText(text);
			builder.setView(TextEditor);
			AlertDialog ad = builder.create(); 
			ad.show();
			TextEditor.requestFocus();
            InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.showSoftInput(TextEditor, 0);
		}
	}
	static DialogMessage mDialogMessage = new DialogMessage();

    protected static View mTextEdit = null;
    SharedPreferences Sp;
	
	static Handler msgHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			sInstance.handleMessage(msg);
		}
	};
	
	public void handleMessage(Message msg) {
		
	}
	
	static public void ShowMessageBox(final String title, final String text, final String[] Buttons) {
		mDialogMessage.Init(title, text, Buttons);
		msgHandler.post(new Runnable() {
			@Override
			public void run() {
				mDialogMessage.ShowMessageBox();
			}
		});
	}
	
	static public void ShowInputBox(final String title, final String prompt, final String text, final String[] Buttons) {
		mDialogMessage.Init(title, prompt, Buttons);
		msgHandler.post(new Runnable() {
			@Override
			public void run() {
				mDialogMessage.ShowInputBox(text);
			}
		});
	}
	
    static class ShowTextInputTask implements Runnable {
        /*
         * This is used to regulate the pan&scan method to have some offset from
         * the bottom edge of the input region and the top edge of an input
         * method (soft keyboard)
         */
        static final int HEIGHT_PADDING = 15;

        public int x, y, w, h;

        public ShowTextInputTask(int x, int y, int w, int h) {
            this.x = x;
            this.y = y;
            this.w = w;
            this.h = h;
        }

		@Override
        public void run() {
			FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(w, h + HEIGHT_PADDING);
			params.leftMargin = x;
			params.topMargin = y;

            if (mTextEdit == null) {
                mTextEdit = new DummyEdit(getContext());

                sInstance.mFrameLayout.addView(mTextEdit, params);
            } else {
                mTextEdit.setLayoutParams(params);
            }

            mTextEdit.setVisibility(View.VISIBLE);
            mTextEdit.requestFocus();

            InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.showSoftInput(mTextEdit, 0);
        }
    }
    
	static public void showTextInput(int x, int y, int w, int h) {
		msgHandler.post(new ShowTextInputTask(x, y, w, h));
	}
	static public void hideTextInput() {
		msgHandler.post(new Runnable() {
			@Override
			public void run() {
		        if (mTextEdit != null) {
		            mTextEdit.setVisibility(View.GONE);
		            InputMethodManager imm = (InputMethodManager) sInstance.getSystemService(Context.INPUT_METHOD_SERVICE);
		            imm.hideSoftInputFromWindow(mTextEdit.getWindowToken(), 0);
		        }
			}
		});
	}
	
	static private native void onMessageBoxOK(int nButton);
	static private native void onMessageBoxText(String text);
	static private native void onNativeExit();
	static public native void onNativeInit();
	static public native void onBannerSizeChanged(int w, int h);
	static private native void initDump(String path);
	static private native void nativeOnLowMemory();
	
	static public void MessageController(int what, int arg1, int arg2) {
        Message msg = msgHandler.obtainMessage();
        msg.what = what;
        msg.arg1 = arg1;
        msg.arg2 = arg2;
		msgHandler.sendMessage(msg);
	}
	
	static public String GetVersion() {
		String verstr = null;
		try {
			verstr = sInstance.getPackageManager().getPackageInfo(sInstance.getPackageName(), 0).versionName;
		} catch (NameNotFoundException e1) {
		}
		return verstr;
	}
	
	StorageManager mStorageManager = null;
    Method mMethodGetPaths = null;
    Method mGetVolumeState = null;
    
    public String[] getStoragePath() {
    	String[] ret = new String[0];
    	if(mStorageManager == null) {
        	mStorageManager = (StorageManager)getSystemService(STORAGE_SERVICE);
            try {
                mMethodGetPaths = StorageManager.class.getMethod("getVolumePaths");
                mGetVolumeState = StorageManager.class.getMethod("getVolumeState", String.class);
            } catch (NoSuchMethodException e) {
                e.printStackTrace();
            }
    	}
    	if(mMethodGetPaths != null) {
            try {
            	ret = (String[])mMethodGetPaths.invoke(mStorageManager);
            } catch (IllegalArgumentException e) {
     
            } catch (IllegalAccessException e) {
     
            } catch (InvocationTargetException e) {
     
            } catch (Exception e) {
     
            }
    	}
        
        if(mGetVolumeState != null) {
            try {
            	for(int i = 0; i < ret.length; ++i) {
            		String status = (String)mGetVolumeState.invoke(mStorageManager, ret[i]);
            		if(Environment.MEDIA_MOUNTED.equals(status) || Environment.MEDIA_MOUNTED_READ_ONLY.equals(status)) {
            			;
            		} else {
            			ret[i] = null;
            		}
            	}
            } catch (IllegalArgumentException e) {
     
            } catch (IllegalAccessException e) {
     
            } catch (InvocationTargetException e) {
     
            } catch (Exception e) {
     
            }
        }
        
		return ret;
    }
    
    private static native void nativeTouchesBegin(final int id, final float x, final float y);
    private static native void nativeTouchesEnd(final int id, final float x, final float y);
    private static native void nativeTouchesMove(final int[] ids, final float[] xs, final float[] ys);
    private static native void nativeTouchesCancel(final int[] ids, final float[] xs, final float[] ys);
    public static native boolean nativeKeyAction(final int keyCode, final boolean isPress);
    public static native void nativeCharInput(final int keyCode);
    public static native void nativeCommitText(String text, int newCursorPosition);
    
    private static native void nativeInsertText(final String text);
    public static native void nativeDeleteBackward();
    private static native String nativeGetContentText();
    private static native void nativeHoverMoved(final float x, final float y);
    private static native void nativeMouseScrolled(final float scroll);
    
    class KR2GLSurfaceView extends Cocos2dxGLSurfaceView {

        public KR2GLSurfaceView(final Context context) {
            super(context);
        }

        public KR2GLSurfaceView(final Context context, final AttributeSet attrs) {
            super(context, attrs);
        }
        
        @Override
        public void insertText(final String pText) {
        	nativeInsertText(pText);
        }

        @Override
        public void deleteBackward() {
        	nativeDeleteBackward();
        }

        @Override
        public boolean onKeyDown(final int pKeyCode, final KeyEvent pKeyEvent) {
            switch (pKeyCode) {
                case KeyEvent.KEYCODE_BACK:
                case KeyEvent.KEYCODE_MENU:
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                case KeyEvent.KEYCODE_ENTER:
                case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
                case KeyEvent.KEYCODE_DPAD_CENTER:
                	nativeKeyAction(pKeyCode, true);
                    return true;
                default:
                    return super.onKeyDown(pKeyCode, pKeyEvent);
            }
        }

        @Override
        public boolean onKeyUp(final int pKeyCode, final KeyEvent pKeyEvent) {
            switch (pKeyCode) {
                case KeyEvent.KEYCODE_BACK:
                case KeyEvent.KEYCODE_MENU:
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                case KeyEvent.KEYCODE_ENTER:
                case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
                case KeyEvent.KEYCODE_DPAD_CENTER:
                	nativeKeyAction(pKeyCode, false);
                    return true;
                default:
                    return super.onKeyUp(pKeyCode, pKeyEvent);
            }
        }
        
        @Override
        public boolean onHoverEvent(final MotionEvent pMotionEvent) {
            final int pointerNumber = pMotionEvent.getPointerCount();
            final float[] xs = new float[pointerNumber];
            final float[] ys = new float[pointerNumber];
            for (int i = 0; i < pointerNumber; i++) {
                xs[i] = pMotionEvent.getX(i);
                ys[i] = pMotionEvent.getY(i);
            }
            
        	switch(pMotionEvent.getActionMasked()) {
        	case MotionEvent.ACTION_HOVER_MOVE:
        		nativeHoverMoved(xs[0], ys[0]);
        		break;
        	}
        	return true;
        }
        
        @Override
        public boolean onTouchEvent(final MotionEvent pMotionEvent) {
        	
            // these data are used in ACTION_MOVE and ACTION_CANCEL
            final int pointerNumber = pMotionEvent.getPointerCount();
            final int[] ids = new int[pointerNumber];
            final float[] xs = new float[pointerNumber];
            final float[] ys = new float[pointerNumber];

            for (int i = 0; i < pointerNumber; i++) {
                ids[i] = pMotionEvent.getPointerId(i);
                xs[i] = pMotionEvent.getX(i);
                ys[i] = pMotionEvent.getY(i);
            }

            switch (pMotionEvent.getAction() & MotionEvent.ACTION_MASK) {
                case MotionEvent.ACTION_POINTER_DOWN:
                    final int indexPointerDown = pMotionEvent.getAction() >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
                    final int idPointerDown = pMotionEvent.getPointerId(indexPointerDown);
                    final float xPointerDown = pMotionEvent.getX(indexPointerDown);
                    final float yPointerDown = pMotionEvent.getY(indexPointerDown);
                    nativeTouchesBegin(idPointerDown, xPointerDown, yPointerDown);
                    break;

                case MotionEvent.ACTION_DOWN:
                    // there are only one finger on the screen
                    final int idDown = pMotionEvent.getPointerId(0);
                    final float xDown = xs[0];
                    final float yDown = ys[0];
                    nativeTouchesBegin(idDown, xDown, yDown);
                    break;

                case MotionEvent.ACTION_MOVE:
                	nativeTouchesMove(ids, xs, ys);
                    break;

                case MotionEvent.ACTION_POINTER_UP:
                    final int indexPointUp = pMotionEvent.getAction() >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
                    final int idPointerUp = pMotionEvent.getPointerId(indexPointUp);
                    final float xPointerUp = pMotionEvent.getX(indexPointUp);
                    final float yPointerUp = pMotionEvent.getY(indexPointUp);
                    nativeTouchesEnd(idPointerUp, xPointerUp, yPointerUp);
                    break;

                case MotionEvent.ACTION_UP:
                    // there are only one finger on the screen
                    final int idUp = pMotionEvent.getPointerId(0);
                    final float xUp = xs[0];
                    final float yUp = ys[0];
                    nativeTouchesEnd(idUp, xUp, yUp);
                    break;

                case MotionEvent.ACTION_CANCEL:
                	nativeTouchesCancel(ids, xs, ys);
                    break;
            }

            /*
            if (BuildConfig.DEBUG) {
                Cocos2dxGLSurfaceView.dumpMotionEvent(pMotionEvent);
            }
            */
            return true;
        }
        
		@TargetApi(Build.VERSION_CODES.HONEYCOMB_MR1) @Override  
        public boolean onGenericMotionEvent(MotionEvent event) {
        	switch (event.getActionMasked()) {
	        case MotionEvent.ACTION_SCROLL:
	        	float v = event.getAxisValue(MotionEvent.AXIS_VSCROLL);
	        	nativeMouseScrolled(-v);
                return true;
            default:
            	break;
        	}
        	return super.onGenericMotionEvent(event);
        }
    }
    
    @Override
    public Cocos2dxGLSurfaceView onCreateView() {
        Cocos2dxGLSurfaceView glSurfaceView = new KR2GLSurfaceView(this);
    	hideSystemUI();
        //this line is need on some device if we specify an alpha bits
        if(this.mGLContextAttrs[3] > 0) glSurfaceView.getHolder().setFormat(PixelFormat.TRANSLUCENT);

        Cocos2dxEGLConfigChooser chooser = new Cocos2dxEGLConfigChooser(this.mGLContextAttrs);
        glSurfaceView.setEGLConfigChooser(chooser);

        return glSurfaceView;
    }
    
    public int get_res_sd_operate_step() { return -1; }

    static void requireLEXA(final String path) {
		msgHandler.post(new Runnable() {
			@Override
			public void run() {
				guideDialogForLEXA(path);
			}
		});
    }
    static void guideDialogForLEXA(final String path) {
    	AlertDialog.Builder builder = new AlertDialog.Builder(sInstance);
    	ImageView image = new ImageView(sInstance);
    	image.setImageResource(sInstance.get_res_sd_operate_step());
    	builder
    		.setView(image)
    		.setTitle(path)
    		.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    triggerStorageAccessFramework();
                }
            })
            .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                	// nothing to do
                }
            })
            .show();
    }
    
    static final boolean isWritable(final File file) {
        if(file==null)
            return false;
        boolean isExisting = file.exists();

        try {
            FileOutputStream output = new FileOutputStream(file, true);
            try {
                output.close();
            }
            catch (IOException e) {
                // do nothing.
            }
        }
        catch (FileNotFoundException e) {
            return false;
        }
        boolean result = file.canWrite();

        // Ensure that file is not created during this process.
        if (!isExisting) {
            file.delete();
        }

        return result;
    }
    
    static final boolean isWritableNormalOrSaf(final String path) {
    	Context c = sInstance;
    	File folder = new File(path);
        if (!folder.exists() || !folder.isDirectory()) {
            return false;
        }

        // Find a non-existing file in this directory.
        int i = 0;
        File file;
        do {
            String fileName = "AugendiagnoseDummyFile" + (++i);
            file = new File(folder, fileName);
        }
        while (file.exists());

        // First check regular writability
        if (isWritable(file)) {
            return true;
        }

        // Next check SAF writability.
        DocumentFile document = getDocumentFile(file, false,c);

        if (document == null) {
            return false;
        }

        // This should have created the file - otherwise something is wrong with access URL.
        boolean result = document.canWrite() && file.exists();

        // Ensure that the dummy file is not remaining.
        document.delete();

        return result;
    }
    
    @TargetApi(Build.VERSION_CODES.KITKAT)
    private static String[] getExtSdCardPaths(Context context) {
        List<String> paths = new ArrayList<String>();
        for (File file : context.getExternalFilesDirs("external")) {
            if (file != null && !file.equals(context.getExternalFilesDir("external"))) {
                int index = file.getAbsolutePath().lastIndexOf("/Android/data");
                if (index < 0) {
                    Log.w("FileUtils", "Unexpected external file dir: " + file.getAbsolutePath());
                } else {
                    String path = file.getAbsolutePath().substring(0, index);
                    try {
                        path = new File(path).getCanonicalPath();
                    }
                    catch (IOException e) {
                        // Keep non-canonical path.
                    }
                    paths.add(path);
                }
            }
        }
        //if(paths.isEmpty())paths.add("/storage/sdcard1");
        return paths.toArray(new String[0]);
    }
    static String[] _extSdPaths;

    public static String getExtSdCardFolder(final File file,Context context) {
    	if(_extSdPaths == null)
    		_extSdPaths = getExtSdCardPaths(context);
        try {
            for (int i = 0; i < _extSdPaths.length; i++) {
                if (file.getCanonicalPath().startsWith(_extSdPaths[i])) {
                    return _extSdPaths[i];
                }
            }
        }
        catch (IOException e) {
            return null;
        }
        return null;
    }
    
    public static boolean isOnExtSdCard(final File file,Context c) {
        return getExtSdCardFolder(file,c) != null;
    }
    
    public static DocumentFile getDocumentFile(final File file, final boolean isDirectory,Context context) {
        String baseFolder = getExtSdCardFolder(file,context);
        boolean originalDirectory=false;
        if (baseFolder == null) {
            return null;
        }

        String relativePath = null;
        try {
            String fullPath = file.getCanonicalPath();
            if(!baseFolder.equals(fullPath))
            relativePath = fullPath.substring(baseFolder.length() + 1);
            else originalDirectory=true;
        }
        catch (IOException e) {
            return null;
        }
        catch (Exception f){
            originalDirectory=true;
            //continue
        }
        String as=PreferenceManager.getDefaultSharedPreferences(context).getString("URI",null);

        Uri treeUri =null;
        if(as!=null)treeUri=Uri.parse(as);
        if (treeUri == null) {
            return null;
        }

        // start with root of SD card and then parse through document tree.
        DocumentFile document = DocumentFile.fromTreeUri(context, treeUri);
        if(originalDirectory)return document;
        String[] parts = relativePath.split("\\/");
        for (int i = 0; i < parts.length; i++) {
            DocumentFile nextDocument = document.findFile(parts[i]);
            if (nextDocument == null) {
            	try {
	                if ((i < parts.length - 1) || isDirectory) {
	                    nextDocument = document.createDirectory(parts[i]);
	                } else {
	                    nextDocument = document.createFile("image", parts[i]);
	                }
	            } catch (Exception e) {
	            	return null;
	            }
            }
            document = nextDocument;
        }

        return document;
    }
    
    static public boolean RenameFile(String from, String to) {
    	File file = new File(from);
    	File target = new File(to);
    	if(!file.exists())
    		return false;
    	if(target.exists()) {
    		if(!DeleteFile(target.getAbsolutePath())) return false;
    	}
    	
    	File parent = target.getParentFile();
    	if(!parent.exists()) {
    		if(!CreateFolders(parent.getAbsolutePath())) return false;
    	}
    	// Try the normal way
        if(file.renameTo(target)) return true;
        
        // Try with Storage Access Framework.
        if (Build.VERSION.SDK_INT>=Build.VERSION_CODES.LOLLIPOP /*&& isOnExtSdCard(file, sInstance)*/) {
            DocumentFile document = getDocumentFile(file, false, sInstance);
        	if(document.renameTo(to))
        		return true;
        }
        
        // Try Media Store Hack
        if (Build.VERSION.SDK_INT==Build.VERSION_CODES.KITKAT) {
        	try {
				FileInputStream input = new FileInputStream(file);
	        	int filesize = (int) file.length();
				byte []buffer = new byte[filesize];
				input.read(buffer);
				input.close();
            	OutputStream out = MediaStoreHack.getOutputStream(sInstance, target.getAbsolutePath());
                out.write(buffer);
                out.close();
                return MediaStoreHack.delete(sInstance, file);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				return false;
				//e.printStackTrace();
			}
        }
        
    	return false;
    }
    
    public static final boolean deleteFilesInFolder(final File folder,Context context) {
        boolean totalSuccess = true;
        if(folder==null)
            return false;
        if (folder.isDirectory()) {
            for (File child : folder.listFiles()) {
                deleteFilesInFolder(child, context);
            }

            if (!folder.delete())
                totalSuccess = false;
        } else {

            if (!folder.delete())
                totalSuccess = false;
        }
        return totalSuccess;
    }

    static public boolean DeleteFile(String path) {
    	File file = new File(path);
    	// First try the normal deletion.
        boolean fileDelete = deleteFilesInFolder(file, sInstance);
        if (file.delete() || fileDelete)
            return true;

        // Try with Storage Access Framework.
        if (Build.VERSION.SDK_INT>=Build.VERSION_CODES.LOLLIPOP && isOnExtSdCard(file, sInstance)) {

            DocumentFile document = getDocumentFile(file, false,sInstance);
            return document.delete();
        }

        // Try the Kitkat workaround.
        if (Build.VERSION.SDK_INT==Build.VERSION_CODES.KITKAT) {
            ContentResolver resolver = sInstance.getContentResolver();

            try {
                Uri uri = MediaStoreHack.getUriFromFile(file.getAbsolutePath(),sInstance);
                resolver.delete(uri, null, null);
                return !file.exists();
            }
            catch (Exception e) {
                Log.e("FileUtils", "Error when deleting file " + file.getAbsolutePath(), e);
                return false;
            }
        }

        return !file.exists();
    }
    
	public static OutputStream getOutputStream(@NonNull final File target,Context context,long s)throws Exception {
	    OutputStream outStream = null;
	    try {
	        // First try the normal way
			if (isWritable(target)) {
			    // standard way
			    outStream = new FileOutputStream(target);
			} else {
			    if (Build.VERSION.SDK_INT>=Build.VERSION_CODES.LOLLIPOP) {
			        // Storage Access Framework
			    DocumentFile targetDocument = getDocumentFile(target, false,context);
			    outStream = context.getContentResolver().openOutputStream(targetDocument.getUri());
			} else if (Build.VERSION.SDK_INT==Build.VERSION_CODES.KITKAT) {
			    // Workaround for Kitkat ext SD card
		        return MediaStoreHack.getOutputStream(context,target.getPath());
		        }
		    }
		} catch (Exception e) {
		    Log.e("FileUtils",
    			"Error when copying file from " +  target.getAbsolutePath(), e);
	    }
	  return outStream;
    }

    
    static public boolean WriteFile(String path, byte data[]) {
        File target = new File(path);
        if(target.exists()) {
        	DeleteFile(target.getAbsolutePath()); // to avoid number suffix name
        } else {
            File parent = target.getParentFile();
            if(!parent.exists())
            	CreateFolders(parent.getAbsolutePath());
        }
        OutputStream out = null;
        
    	// Try the normal way
    	try {
        	if(isWritable(target)) {
        		OutputStream os = new FileOutputStream(target);
    			os.write(data);
    			os.close();
    			return true;
        	}

            // Try with Storage Access Framework.
            if (Build.VERSION.SDK_INT>=Build.VERSION_CODES.LOLLIPOP /*&& isOnExtSdCard(file, sInstance)*/) {
                DocumentFile document = getDocumentFile(target, false, sInstance);
                try {
                	Uri docUri = document.getUri();
                    out = sInstance.getContentResolver().openOutputStream(docUri);
                } catch (FileNotFoundException e) {
                    // e.printStackTrace();
                } catch (IOException e) {
                    // e.printStackTrace();
                }
            } else if (Build.VERSION.SDK_INT==Build.VERSION_CODES.KITKAT) {
                // Workaround for Kitkat ext SD card
                Uri uri = MediaStoreHack.getUriFromFile(target.getAbsolutePath(),sInstance);
                out = sInstance.getContentResolver().openOutputStream(uri);
            } else {
                return false;
            }
            
            if (out != null) {
                out.write(data);
                out.close();
                return true;
            }
		} catch (FileNotFoundException e) {
			//return false;
		} catch (IOException e) {
			//return false;
		}

    	return false;
    }
    
    static public boolean CreateFolders(String path) {
    	File file = new File(path);
    	
        // Try the normal way
    	if(file.mkdirs()) {
    		return true;
    	}

        // Try with Storage Access Framework.
        if (Build.VERSION.SDK_INT>=Build.VERSION_CODES.LOLLIPOP /*&& FileUtil.isOnExtSdCard(file, context)*/) {
            DocumentFile document = getDocumentFile(file, true,sInstance);
            // getDocumentFile implicitly creates the directory.

            return document.exists();
        }
        
        // Try the Kitkat workaround.
        if (Build.VERSION.SDK_INT==Build.VERSION_CODES.KITKAT) {
            try {
            	return MediaStoreHack.mkdir(sInstance,file);
            } catch (IOException e) {
                //return false;
            }
        }
        
    	return false;
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        //SDLActivity.mHasFocus = hasFocus;
        if (hasFocus) {
        	hideSystemUI();
        }
    }
    
    @TargetApi(Build.VERSION_CODES.HONEYCOMB)
    void doSetSystemUiVisibility() {
		int uiOpts = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
		        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
		        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
		        | View.SYSTEM_UI_FLAG_FULLSCREEN
		        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
		        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
		getWindow().getDecorView().setSystemUiVisibility(uiOpts);
    }

    private static native boolean nativeGetHideSystemButton();
    void hideSystemUI() {
    	if(nativeGetHideSystemButton() && android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT) {
    		doSetSystemUiVisibility();
    	}
    }
    
    static public String getLocaleName() {
    	Locale defloc = Locale.getDefault();
    	String lang = defloc.getLanguage();
    	String country = defloc.getCountry();
    	if(!country.isEmpty()) {
    		lang += "_";
    		lang += country.toLowerCase();
    	}
    	return lang;
    }
    
    static public void exit() {
    	System.exit(0);
    }
    
    static final int ORIENT_VERTICAL = 1;
    static final int ORIENT_HORIZONTAL = 2;
    
    static public void setOrientation(int orient) {
    	if(orient == ORIENT_VERTICAL) {
    		sInstance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
    	} else if(orient == ORIENT_HORIZONTAL) {
    		sInstance.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
    	}
    }
    
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
	static public void triggerStorageAccessFramework() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        sInstance.startActivityForResult(intent, 3);
    }

    @TargetApi(Build.VERSION_CODES.KITKAT)
    protected void onActivityResult(int requestCode, int responseCode, Intent intent) {
        if (requestCode == 3) {
            String p = Sp.getString("URI", null);
            Uri oldUri = null;
            if (p != null) oldUri = Uri.parse(p);
            Uri treeUri = null;
            if (responseCode == Activity.RESULT_OK) {
                // Get Uri from Storage Access Framework.
                treeUri = intent.getData();
                // Persist URI - this is required for verification of writability.
                if (treeUri != null) Sp.edit().putString("URI", treeUri.toString()).commit();
            }

            // If not confirmed SAF, or if still not writable, then revert settings.
            if (responseCode != Activity.RESULT_OK) {
               /* DialogUtil.displayError(getActivity(), R.string.message_dialog_cannot_write_to_folder_saf, false,
                        currentFolder);||!FileUtil.isWritableNormalOrSaf(currentFolder)
*/
                if (treeUri != null) Sp.edit().putString("URI", oldUri.toString()).commit();
                return;
            }

            // After confirmation, update stored value of folder.
            // Persist access permissions.
            final int takeFlags = intent.getFlags()
                    & (Intent.FLAG_GRANT_READ_URI_PERMISSION
                    | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            getContentResolver().takePersistableUriPermission(treeUri, takeFlags);
        }
    }

}
