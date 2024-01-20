package nirenr.androlua;


import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Insets;
import android.graphics.Movie;
import android.graphics.NinePatch;
import android.graphics.Outline;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.Region;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.NinePatchDrawable;
import android.os.Build;
import android.os.Handler;
import android.util.AttributeSet;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;

import nirenr.androlua.util.AsyncTaskX;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Created by nirenr on 2018/09/05 0005.
 */

public class LuaBitmapDrawable extends Drawable implements Runnable, LuaGcable {

    private LuaContext mLuaContext;
    private int mDuration;
    private long mMovieStart;
    private int mCurrentAnimationTime;
    private Movie mMovie;
    private LoadingDrawable mLoadingDrawable;
    private Drawable mBitmapDrawable;
    private NineBitmapDrawable mNineBitmapDrawable;
    private ColorFilter mColorFilter;
    private int mFillColor;
    private int mScaleType = FIT_XY;
    private GifDecoder mGifDecoder;
    private GifDecoder mGifDecoder2;
    private Handler mHandler;
    private GifDecoder.GifFrame mGifFrame;
    private int mDelay;
    private boolean mGc;
    private int mAlpha = 255;
    private boolean mHasInvalidate;
    private NinePatchDrawable mNinePatchDrawable;

    public static void setCacheTime(long time) {
        mCacheTime = time;
    }

    public static long getCacheTime() {
        return mCacheTime;
    }

    private static long mCacheTime = 7 * 24 * 60 * 60 * 1000;

    public LuaBitmapDrawable(LuaContext context, String path, Drawable def) {
        this(context, path);
        mBitmapDrawable = def;
    }

    public LuaBitmapDrawable(LuaContext context, String path) {
        context.regGc(this);
        mLuaContext = context;
        mLoadingDrawable = new LoadingDrawable(context.getContext());
        if (path.toLowerCase().startsWith("http://") || path.toLowerCase().startsWith("https://")) {
            initHttp(context, path);
        } else {
            if (!path.startsWith("/")) {
                path = context.getLuaPath(path);
            }
            init(path);
        }
        //Log.i("rime", "backget:init gif " + mGifDecoder2 + " bmp " + mBitmapDrawable + " 9p " + mNineBitmapDrawable + path);
    }

    private void initHttp(final LuaContext context, final String path) {
        new AsyncTaskX<String, String, String>() {
            @Override
            protected String doInBackground(String... strings) {
                try {
                    return getHttpBitmap(context, path);
                } catch (Exception e) {
                    e.printStackTrace();
                }
                return "";
            }

            @Override
            protected void onPostExecute(String s) {
                init(s);
            }
        }.execute();
    }

    private void init(final String path) {
        //Log.i("rime", "backget:init1 gif " + mGifDecoder2 + " bmp " + mBitmapDrawable + " 9p " + mNineBitmapDrawable + path);
        if (path.endsWith("png") || path.endsWith("jpg")) {
            init2(path);
            return;
        }
        try {
            mGifDecoder = new GifDecoder(new FileInputStream(path), new GifDecoder.GifAction() {
                @Override
                public void parseOk(boolean parseStatus, int frameIndex) {
                    if (!parseStatus && frameIndex < 0) {
                        init2(path);
                    } else if (parseStatus && mGifDecoder2 == null && mGifDecoder.getFrameCount() > 1) {     //当帧数大于1时，启动动画线程
                        mGifDecoder2 = mGifDecoder;
                    }

                }
            });
            mGifDecoder.start();
        } catch (Exception e) {
            e.printStackTrace();
            init2(path);
        }
        //Log.i("rime", "backget:init11 gif " + mGifDecoder2+" bmp " + mBitmapDrawable+" 9p " + mNineBitmapDrawable+path);

    }


    private void init2(String path) {
        //Log.i("rime", "backget:init2 gif " + mGifDecoder2+" bmp " + mBitmapDrawable+" 9p " + mNineBitmapDrawable+path);
        if (path.isEmpty()) {
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    mLoadingDrawable.setState(-1);
                }
            }, 1000);
            invalidateSelf();
            return;
        }

        if (path.contains(".9.png")) {
            try {
                final Bitmap bitmap = BitmapFactory.decodeFile(path);
                final byte[] chunk = bitmap.getNinePatchChunk();
                // 如果 .9.png 没有经过第一步，那么 chunk 就是 null, 只能按照普通方式加载
                if (NinePatch.isNinePatchChunk(chunk)) {
                    mNinePatchDrawable = new NinePatchDrawable(Resources.getSystem(), bitmap, chunk, loadCompiled(chunk), null);
                    invalidateSelf();
                    return;
                }
                bitmap.recycle();
            }catch (Exception e){
                //e.printStackTrace();
            }
            try {
                mNineBitmapDrawable = new NineBitmapDrawable(path);
                invalidateSelf();
                return;
            } catch (Exception e) {
                //e.printStackTrace();
                try {
                    Bitmap bmp = LuaBitmap.getLocalBitmap(path);
                    int w = bmp.getWidth();
                    int h = bmp.getHeight();
                    mNineBitmapDrawable = new NineBitmapDrawable(bmp, w / 4, h / 4, w / 4 * 3, h / 4 * 3);
                    invalidateSelf();
                    return;
                } catch (Exception ex) {
                    ex.printStackTrace();
                }
            }
        } else {
            try {
                mBitmapDrawable = new BitmapDrawable(LuaBitmap.getLocalBitmap(mLuaContext, path));
                invalidateSelf();
                return;
            } catch (Exception e1) {
                e1.printStackTrace();
            }
        }


        if (mBitmapDrawable == null && mNineBitmapDrawable == null) {
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    mLoadingDrawable.setState(-1);
                }
            }, 1000);
        }
        invalidateSelf();
        //Log.i("rime", "backget:init22 gif " + mGifDecoder2+" bmp " + mBitmapDrawable+" 9p " + mNineBitmapDrawable+path);

    }

    private Rect loadCompiled(byte[] bArr) {
        ByteBuffer order = ByteBuffer.wrap(bArr).order(ByteOrder.nativeOrder());
        order.get();
        order.get();
        order.get();
        order.get();
        order.getInt();
        order.getInt();
        int mL = order.getInt();
        int mR = order.getInt();
        int mT = order.getInt();
        int mB = order.getInt();
        order.getInt();
        int mX1 = order.getInt();
        int mX2 = order.getInt();
        int mY1 = order.getInt();
        int mY2 = order.getInt();
        return new Rect(mL, mT, mR, mB);
    }

    @Override
    public void getOutline(Outline outline) {
        if (mNinePatchDrawable != null) {
            mNinePatchDrawable.getOutline(outline);
            return;
        }
        super.getOutline(outline);
    }

    @Override
    public int getAlpha() {
        return mAlpha;
    }


    @Override
    public void setAutoMirrored(boolean mirrored) {
        if (mNinePatchDrawable != null) {
            mNinePatchDrawable.setAutoMirrored(mirrored);
        }
        super.setAutoMirrored(mirrored);
    }


    @Override
    public boolean isAutoMirrored() {
        if (mNinePatchDrawable != null) {
            return mNinePatchDrawable.isAutoMirrored();
        }
        return super.isAutoMirrored();
    }

    @Override
    public void setFilterBitmap(boolean filter) {
        if (mNinePatchDrawable != null) {
            mNinePatchDrawable.setFilterBitmap(filter);
            return;
        }
        super.setFilterBitmap(filter);
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    public boolean isFilterBitmap() {
        if (mNinePatchDrawable != null) {
            return mNinePatchDrawable.isFilterBitmap();
        }
        return super.isFilterBitmap();
    }

    @Override
    public void inflate(Resources r, XmlPullParser parser, AttributeSet attrs, Resources.Theme theme)
            throws XmlPullParserException, IOException {
        super.inflate(r, parser, attrs, theme);
        if (mNinePatchDrawable != null) {
            mNinePatchDrawable.inflate(r, parser, attrs, theme);
        }
    }

    @Override
    public int getOpacity() {
        if (mNinePatchDrawable != null) {
            return mNinePatchDrawable.getOpacity();
        }
        return PixelFormat.UNKNOWN;
    }

    @Override
    public Region getTransparentRegion() {
        if (mNinePatchDrawable != null) {
            return mNinePatchDrawable.getTransparentRegion();
        }
        return super.getTransparentRegion();
    }

    @Override
    public ConstantState getConstantState() {
        if (mNinePatchDrawable != null) {
            return mNinePatchDrawable.getConstantState();
        }
        return super.getConstantState();
    }

    @Override
    public Drawable mutate() {
        return this;
    }


    @Override
    public boolean isStateful() {
        if (mNinePatchDrawable != null) {
            return mNinePatchDrawable.isStateful();
        }
        return super.isStateful();
    }


    @RequiresApi(api = Build.VERSION_CODES.Q)
    @NonNull
    @Override
    public Insets getOpticalInsets() {
        if (mNinePatchDrawable != null) {
            return mNinePatchDrawable.getOpticalInsets();
        }
        return super.getOpticalInsets();
    }

    @Override
    public int getIntrinsicWidth() {
        if (mNinePatchDrawable != null) {
            return mNinePatchDrawable.getIntrinsicWidth();
        } else if (mNineBitmapDrawable != null) {
            return mNineBitmapDrawable.getIntrinsicWidth();
        }
        return super.getIntrinsicWidth();
    }

    @Override
    public int getIntrinsicHeight() {
        if (mNinePatchDrawable != null) {
            return mNinePatchDrawable.getIntrinsicHeight();
        } else if (mNineBitmapDrawable != null) {
            return mNineBitmapDrawable.getIntrinsicHeight();
        }
        return super.getIntrinsicHeight();
    }

    @Override
    public boolean getPadding(@NonNull Rect padding) {
        if (mNinePatchDrawable != null) {
            //Log.i("trime", "getPadding:1 "+mNinePatchDrawable.getPadding(padding));
            return mNinePatchDrawable.getPadding(padding);
        } else if (mNineBitmapDrawable != null) {
            //Log.i("trime", "getPadding:2 "+mNineBitmapDrawable.getPadding(padding));
            return mNineBitmapDrawable.getPadding(padding);
        }
        //Log.i("trime", "getPadding:3 ");
        return super.getPadding(padding);
    }

    public int getWidth() {
        if (mMovie != null) {
            return mMovie.width();
        } else if (mBitmapDrawable != null) {
            return mBitmapDrawable.getIntrinsicWidth();
        } else if (mNinePatchDrawable != null) {
            return mNinePatchDrawable.getIntrinsicWidth();
        } else if (mNineBitmapDrawable != null) {
            return mNineBitmapDrawable.getIntrinsicWidth();
        } else if (mGifDecoder != null) {
            return mGifDecoder.width;
        }
        return super.getIntrinsicWidth();
    }

    public int getHeight() {
        if (mMovie != null) {
            return mMovie.height();
        } else if (mBitmapDrawable != null) {
            return mBitmapDrawable.getIntrinsicHeight();
        } else if (mNinePatchDrawable != null) {
            return mNinePatchDrawable.getIntrinsicWidth();
        } else if (mNineBitmapDrawable != null) {
            return mNineBitmapDrawable.getIntrinsicHeight();
        } else if (mGifDecoder != null) {
            return mGifDecoder.height;
        }
        return super.getIntrinsicHeight();
    }

    public int getWidth(int i) {
        int w = getWidth();
        if (w <= 0)
            return i;
        int h = getHeight();
        //Log.i("rime", "getWidth: " + w + ";" + h + ";" + i + ";" + ";" + (i / h * w));
        return i / h * w;
    }

    @Override
    public void draw(Canvas canvas) {
        canvas.drawColor(mFillColor);
        //Log.i("rime", "backget: gif " + mGifDecoder2 + " bmp " + mBitmapDrawable + " 9p " + mNineBitmapDrawable);
        if (mGifDecoder2 != null) {
            long now = System.currentTimeMillis();
            if (mMovieStart == 0 || mGifFrame == null) {
                mGifFrame = mGifDecoder2.next();
                mDelay = mGifFrame.delay;
                mMovieStart = now;
            } else {
                while (now - mMovieStart > mDelay) {
                    mGifFrame = mGifDecoder2.next();
                    mDelay = mGifFrame.delay;
                    mMovieStart += mDelay;
                }
            }
            if (mGifFrame != null) {
                Rect bound = getBounds();
                BitmapDrawable mBitmapDrawable = new BitmapDrawable(mGifFrame.image);
                int width = mBitmapDrawable.getIntrinsicWidth();
                int height = mBitmapDrawable.getIntrinsicHeight();
                float mScale = 1;
                if (mScaleType == FIT_XY) {
                    float mScaleX = (float) (bound.right - bound.left) / (float) width;
                    float mScaleY = (float) (bound.bottom - bound.top) / (float) height;
                    width = (int) (width * mScaleX);
                    height = (int) (height * mScaleY);
                } else if (mScaleType != MATRIX) {
                    mScale = Math.min((float) (bound.bottom - bound.top) / (float) height, (float) (bound.right - bound.left) / (float) width);
                    width = (int) (width * mScale);
                    height = (int) (height * mScale);
                }
                int left = bound.left;
                int top = bound.top;
                switch (mScaleType) {
                    case FIT_CENTER:
                        left = (int) (((bound.right - bound.left) - width) / 2);
                        top = (int) (((bound.bottom - bound.top) - height) / 2);
                        break;
                    case FIT_END:
                        top = (int) ((bound.bottom - bound.top) - height);
                        break;
                }
                //float mScale = Math.min((float) (bound.bottom - bound.top) / (float) mBitmapDrawable.getIntrinsicHeight(), (float) (bound.right - bound.left) / (float) mBitmapDrawable.getIntrinsicWidth());
                mBitmapDrawable.setBounds(new Rect(left, top, left + width, top + height));
                mBitmapDrawable.setAlpha(mAlpha);
                mBitmapDrawable.setColorFilter(mColorFilter);
                mBitmapDrawable.draw(canvas);
                // canvas.drawBitmap(mGifFrame.image, null, getBounds(), null);
            }
            invalidateSelf();
        } else if (mBitmapDrawable != null) {
            //Log.i("rime", "backget: " + getBounds() + mBitmapDrawable);
            Rect bound = getBounds();
            int width = mBitmapDrawable.getIntrinsicWidth();
            int height = mBitmapDrawable.getIntrinsicHeight();
            float mScale = 1;
            if (mScaleType == FIT_XY) {
                float mScaleX = (float) (bound.right - bound.left) / (float) width;
                float mScaleY = (float) (bound.bottom - bound.top) / (float) height;
                width = (int) (width * mScaleX);
                height = (int) (height * mScaleY);
            } else if (mScaleType != MATRIX) {
                mScale = Math.min((float) (bound.bottom - bound.top) / (float) height, (float) (bound.right - bound.left) / (float) width);
                width = (int) (width * mScale);
                height = (int) (height * mScale);
            }
            int left = bound.left;
            int top = bound.top;
            switch (mScaleType) {
                case FIT_CENTER:
                    left = (int) (((bound.right - bound.left) - width) / 2);
                    top = (int) (((bound.bottom - bound.top) - height) / 2);
                    break;
                case FIT_END:
                    top = (int) ((bound.bottom - bound.top) - height);
                    break;
            }
            //float mScale = Math.min((float) (bound.bottom - bound.top) / (float) mBitmapDrawable.getIntrinsicHeight(), (float) (bound.right - bound.left) / (float) mBitmapDrawable.getIntrinsicWidth());
            mBitmapDrawable.setBounds(new Rect(left, top, left + width, top + height));
            mBitmapDrawable.setAlpha(mAlpha);
            mBitmapDrawable.setColorFilter(mColorFilter);
            mBitmapDrawable.draw(canvas);
            //canvas.drawBitmap(mBitmapDrawable.getBitmap(),getBounds(),getBounds(),new Paint());
            mHasInvalidate = false;
        } else if (mNinePatchDrawable != null) {
            mNinePatchDrawable.setBounds(getBounds());
            mNinePatchDrawable.setAlpha(mAlpha);
            mNinePatchDrawable.setColorFilter(mColorFilter);
            mNinePatchDrawable.draw(canvas);
            mHasInvalidate = false;
        } else if (mNineBitmapDrawable != null) {
            mNineBitmapDrawable.setBounds(getBounds());
            mNineBitmapDrawable.setAlpha(mAlpha);
            mNineBitmapDrawable.setColorFilter(mColorFilter);
            mNineBitmapDrawable.draw(canvas);
            mHasInvalidate = false;
        } else if (mLoadingDrawable != null) {
            mLoadingDrawable.setBounds(getBounds());
            mLoadingDrawable.draw(canvas);
            mLoadingDrawable.setAlpha(mAlpha);
            mLoadingDrawable.setColorFilter(mColorFilter);
            invalidateSelf();
        }
    }

    @Override
    public void invalidateSelf() {
        try {
            mHasInvalidate = true;
            Rect rect = getBounds();
            if (rect.right - rect.left <= 0)
                return;
            super.invalidateSelf();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void finalize() throws Throwable {
        if (mGifDecoder2 != null)
            mGifDecoder2.free();
    }

    public void setScaleType(int scaleType) {

        if (mScaleType != scaleType) {
            mScaleType = scaleType;
            invalidateSelf();
        }
    }

    public void setFillColor(int fillColor) {
        if (fillColor == mFillColor) {
            return;
        }
        mFillColor = fillColor;
    }

    @Override
    public void setAlpha(int alpha) {
        mAlpha = alpha;
    }

    @Override
    public void setColorFilter(ColorFilter colorFilter) {
        mColorFilter = colorFilter;
    }


    public static String getHttpBitmap(LuaContext context, String url) throws IOException {
        //Log.d(TAG, url);
        String path = context.getLuaExtDir("cache") + "/" + url.hashCode();
        File f = new File(path);
        if (f.exists() && System.currentTimeMillis() - f.lastModified() < mCacheTime) {
            return path;
        }
        new File(path).delete();
        URL myFileUrl = new URL(url);
        HttpURLConnection conn = (HttpURLConnection) myFileUrl.openConnection();
        conn.setConnectTimeout(120000);
        conn.setDoInput(true);
        conn.connect();
        InputStream is = conn.getInputStream();
        FileOutputStream out = new FileOutputStream(path);
        if (!LuaUtil.copyFile(is, out)) {
            out.close();
            is.close();
            new File(path).delete();
            throw new RuntimeException("LoadHttpBitmap Error.");
        }
        out.close();
        is.close();
        return path;
    }

    public static final int MATRIX = (0);
    public static final int FIT_XY = (1);
    public static final int FIT_START = (2);
    public static final int FIT_CENTER = (3);
    public static final int FIT_END = (4);
    public static final int CENTER = (5);
    public static final int CENTER_CROP = (6);
    public static final int CENTER_INSIDE = (7);

    @Override
    public void run() {
        invalidateSelf();
    }

    public boolean isHasInvalidate() {
        return mHasInvalidate;
    }

    @Override
    public void gc() {
        mHasInvalidate = false;
        if (isGc())
            return;
        try {
            if (mGifDecoder2 != null)
                mGifDecoder2.free();
            if (mBitmapDrawable != null && mBitmapDrawable instanceof BitmapDrawable) {
                Bitmap bmp = ((BitmapDrawable) mBitmapDrawable).getBitmap();
                if (bmp == null)
                    return;
                LuaBitmap.removeBitmap(bmp);
                if (bmp.isRecycled())
                    return;
                bmp.recycle();
            }
            if (mNinePatchDrawable != null)
                mNinePatchDrawable = null;

            if (mNineBitmapDrawable != null)
                mNineBitmapDrawable.gc();
            mGifDecoder2 = null;
            mBitmapDrawable = null;
            mNineBitmapDrawable = null;
            mLoadingDrawable.setState(-1);
        } catch (Exception e) {
            e.printStackTrace();
        }
        mGc = true;
    }

    @Override
    public boolean isGc() {
        return mGc;
    }

    public boolean isInside(int x, int y) {
        if(mBitmapDrawable!=null&&mBitmapDrawable instanceof BitmapDrawable)
        {
            Bitmap bmp = ((BitmapDrawable) mBitmapDrawable).getBitmap();
            float sx = getBounds().width()*1.0f / bmp.getWidth();
            x= (int) (x/sx);
            float sy = getBounds().height()*1.0f / bmp.getHeight();
            y= (int) (y/sy);
            if(x<0||x>=bmp.getWidth())
                return false;
            if(y<0||y>=bmp.getHeight())
                return false;

            return bmp.getPixel(x,y)!=0;
        }

        return true;
    }
}
