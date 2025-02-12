package nirenr.androlua;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.drawable.Drawable;

import nirenr.luajava.LuaException;
import nirenr.luajava.LuaObject;
import nirenr.luajava.util.LuaFunction;

public class LuaDrawable extends Drawable {

    private final LuaContext mContext;
    private LuaObject mDraw;

    private Paint mPaint;
    private LuaFunction mOnDraw;


    public LuaDrawable(LuaFunction func) {
        mDraw = func;
        mPaint = new Paint();
        mContext = mDraw.getLuaState().getContext();
    }

    @Override
    public void draw(Canvas p1) {
        try {
            if (mOnDraw == null) {
                Object r = mDraw.call(Arrays.asList(p1, mPaint, this).toArray());
                if (r != null && r instanceof LuaFunction)
                    mOnDraw = (LuaFunction) r;
            }
            if (mOnDraw != null) {
                mOnDraw.call(p1);
            }
        } catch (LuaException e) {
            mContext.sendError("onDraw", e);
        }
        // TODO: Implement this method
    }

    @Override
    public void setAlpha(int p1) {
        mPaint.setAlpha(p1);
        // TODO: Implement this method
    }

    @Override
    public void setColorFilter(ColorFilter p1) {
        mPaint.setColorFilter(p1);
        // TODO: Implement this method
    }

    @Override
    public int getOpacity() {
        // TODO: Implement this method
        return PixelFormat.UNKNOWN;
    }

    public Paint getPaint() {
        return mPaint;
    }
}
