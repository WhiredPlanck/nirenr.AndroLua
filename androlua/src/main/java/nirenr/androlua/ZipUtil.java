package nirenr.androlua;

import java.io.IOException;

public class ZipUtil {
	public static boolean zip(String sourceFilePath, String zipFilePath) {
		return LuaUtil.zip(sourceFilePath, zipFilePath);
	}

	public static boolean unzip(String zipPath, String destPath) {
		try {
			LuaUtil.unZip(zipPath, destPath);
			return true;
		}
		catch (IOException e) {
			return false;
		}
	}
}
