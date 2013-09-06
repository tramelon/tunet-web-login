package cn.edu.tsinghua.zebra;

import java.io.BufferedReader;
import java.io.Console;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintStream;
import java.io.UnsupportedEncodingException;
import java.net.URL;
import java.net.URLConnection;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.DecimalFormat;
import java.util.Arrays;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MyLogin {
	static String serverURL = "net.tsinghua.edu.cn";

	public static void main(String[] args) {
		Console c = System.console();
		if (c == null) {
			System.err.println("No console.");
			System.exit(1);
		}
		try {
			String isonline = postData("http://" + serverURL
					+ "/cgi-bin/do_login", "action=check_online");
			String[] arr;
			if (isonline != null)// 已经在线
			{
				System.out.println("您已经在线，下面是连接信息：");
				arr = isonline.split(",");
				System.out.println("用户名"+arr[1]);
				System.out.println("已用流量" + format_flux(Long.valueOf(arr[2])));
				System.out.println("在线时长"
						+ format_time(Integer.valueOf(arr[4])));
				System.out.println("是否退出[y/N]？");
				BufferedReader in = new BufferedReader(new InputStreamReader(
						System.in));
				if (in.readLine().equals("y"))
					do_logout();
				System.exit(0);
			}
			// 尝试读取缓存的用户名及密码md5值，如果没有则需要重新输入
			String fileName = "MyLogin_Info", uname, pass_md5;
			boolean isFromFile = false;
			try {
				BufferedReader in = new BufferedReader(new FileReader(fileName));
				uname = in.readLine(); // 读取一行内容
				pass_md5 = in.readLine(); // 读取一行内容
				in.close();
				isFromFile = true;
			} catch (IOException iox) {
				BufferedReader in = new BufferedReader(new InputStreamReader(
						System.in));
				System.out.print("输入用户名: ");
				uname = in.readLine();
				char[] Password = c.readPassword("输入密码: ");
				pass_md5 = getMD5Str(Password);
				Arrays.fill(Password, ' ');
				isFromFile = false;
			}
			String data = "username=" + uname + "&password=" + pass_md5
					+ "&drop=" + 0 + "&type=1&n=100";
			String urlstr = "http://" + serverURL + "/cgi-bin/do_login";
			String result = postData(urlstr, data);
			Pattern mPattern = Pattern.compile("^([\\d]+,){4}[\\d]$");
			Matcher mMatch = mPattern.matcher(result);
			Pattern mPattern1 = Pattern.compile("^password_error@[\\d]+");
			Matcher mMatch1 = mPattern1.matcher(result);
			if (mMatch.matches()) {
				arr = result.split(",");
				System.out.println("登陆成功！");
				System.out.println("已用流量" + format_flux(Long.valueOf(arr[2])));
				if (!isFromFile) {
					// 存储用户名及密码
					try {
						FileOutputStream out = new FileOutputStream(fileName);
						PrintStream p = new PrintStream(out);
						p.println(uname);
						p.println(pass_md5);
						out.close();
						System.out.println("用户" + uname + "信息已保存");
					} catch (FileNotFoundException e) {
						System.out.println("无法保存用户信息");
					}
				}
			} else if (mMatch1.matches()) {
				System.out.println("密码错误或会话失效");
				File profile = new File(fileName);
				if (profile.exists())
					profile.deleteOnExit();
			} else {
				System.out.println(result);
			}
		} catch (Exception e) {
			System.err.println("Error.");
			System.exit(1);
		}
	}

	private static void do_logout() throws IOException {
		String res = postData("http://" + serverURL + "/cgi-bin/do_logout", "");
		if (res.equals("logout_ok"))
			System.out.println("连接已断开");
		else
			System.out.println("操作失败");
	}

	private static String format_time(int sec) {
		int h = (int) Math.floor(sec / 3600);
		String hString = String.valueOf(h);
		int m = (int) Math.floor((sec % 3600) / 60);
		String mString = String.valueOf(m);
		int s = sec % 3600 % 60;
		String sString = String.valueOf(s);
		if (h < 10) {
			hString = "0" + h;
		}
		if (m < 10) {
			mString = "0" + m;
		}
		if (s < 10) {
			sString = "0" + s;
		}
		String out = hString + ":" + mString + ":" + sString;
		return out;
	}

	private static String format_flux(long size) {
		/*int temp = 0;
		String[] units = { "B", "K", "M", "G" };
		String flux = "";
		short i = 0;
		if (L==0)
			return "0B";
		while (L != 0) {
			temp = (int) (L & (long) 1023);
			if (i < 4)
				flux = String.valueOf(temp) + units[i] + "," + flux;
			else
				flux = String.valueOf(temp) + " " + flux;
			i++;
			L >>= 10;
		}
		return flux;
		*/
		if(size <= 0) return "0";
	    final String[] units = new String[] { "B", "KB", "MB", "GB", "TB" };
	    int digitGroups = (int) (Math.log10(size)/Math.log10(1024));
	    return new DecimalFormat("#,##0.#").format(size/Math.pow(1024, digitGroups)) + " " + units[digitGroups];
	}

	private static String postData(String urlstr, String data)
			throws IOException {
		URL url = new URL(urlstr);
		URLConnection con = url.openConnection();
		con.setDoOutput(true);
		OutputStreamWriter out = new OutputStreamWriter(con.getOutputStream());
		out.write(data);
		out.flush();
		out.close();
		BufferedReader reader = new BufferedReader(new InputStreamReader(
				con.getInputStream()));
		String row;
		row = reader.readLine();
		return row;
	}

	/*
	 * MD5加密
	 */
	private static String getMD5Str(char[] passwd) {
		MessageDigest messageDigest = null;
		String str;
		str = String.valueOf(passwd);
		try {
			messageDigest = MessageDigest.getInstance("MD5");

			messageDigest.reset();

			messageDigest.update(str.getBytes("UTF-8"));
		} catch (NoSuchAlgorithmException e) {
			System.out.println("NoSuchAlgorithmException caught!");
			System.exit(-1);
		} catch (UnsupportedEncodingException e) {
			e.printStackTrace();
		}

		byte[] byteArray = messageDigest.digest();

		StringBuffer md5StrBuff = new StringBuffer();

		for (int i = 0; i < byteArray.length; i++) {
			if (Integer.toHexString(0xFF & byteArray[i]).length() == 1)
				md5StrBuff.append("0").append(
						Integer.toHexString(0xFF & byteArray[i]));
			else
				md5StrBuff.append(Integer.toHexString(0xFF & byteArray[i]));
		}
		// 16位加密，从第9位到25位
		return md5StrBuff.toString();
	}
}
