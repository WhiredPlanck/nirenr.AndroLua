package nirenr.androlua;

import android.content.Context;
import android.view.View;

import nirenr.luajava.LuaException;
import nirenr.luajava.LuaObject;
import nirenr.luajava.util.LuaTable;

/**
 * Created by Administrator on 2018/08/29 0029.
 */

public class LuaView extends View {

    private LuaTable mTable;
    private LuaObject mOnMeasure;

    public LuaView(Context context){
        super(context);
    }

    public LuaView(Context context, LuaTable table){
        super(context);
        mTable=table;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        if(mTable!=null){
            try {
                mOnMeasure = mTable.getField("onMeasure");
                if(mOnMeasure.isFunction()){
                    mOnMeasure.call(widthMeasureSpec,heightMeasureSpec,this);
                    return;
                }
            } catch (LuaException e) {
                e.printStackTrace();
            }
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }


}
