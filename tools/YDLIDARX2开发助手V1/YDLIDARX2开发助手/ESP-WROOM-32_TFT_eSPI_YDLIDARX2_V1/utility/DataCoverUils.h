/*===================================================
 * 数据转换类
 * Copyright (c) 2025 djq 
 * QQ:912503010
 * 版本:V1.0.0.1 
 * 日期:2025-01-08 10:20:10
 * ===================================================
 */
 
/**
 * 十六进制字符串转成Ascii
 */
 String hexToAscii(String hex)
{
 uint16_t len = hex.length();
String ascii ="";
for (uint16_t i = 0; i < len; i += 2){
ascii += (char)strtol(hex.substring(i,i+2).c_str(),NULL,16);
}
return ascii;
}


/**
 * 十六进制字符串 转换 16位的二进制 l_size为多少字节的bit
 */
String hexToBitXBin(String hexStr,int l_size) {
  char temp[10];
  long decimal = strtol(hexStr.c_str(), NULL, 16); // 将十六进制字符串转换为十进制
  itoa(decimal, temp, 2); // 将十进制数转换为二进制
  String binaryStr = temp;
  // 补充前导零以满足16位二进制数
  while (binaryStr.length() < l_size) {
    binaryStr = "0" + binaryStr;
  }
  return binaryStr;
}


/**
 * 十进制转十六进制 
 */
String intTohex(int n) { 
  if (n == 0){ 
    return "00"; //n为0 
  }

  String result = ""; 
  char _16[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' }; 
  const int radix = 16;
  while (n) {
    int i = n % radix; // 余数 
   result = _16[i] + result; // 将余数对应的十六进制数字加入结果 
   n/= radix; // 除以16获得商，最为下一轮的被除数 
  } 
  
  if (result.length() < 2) { 
    result = '0' + result; //不足两位补零 
    } 
    return result; 
 }
    
/**
 * 将数据右移一位,即：十六进制转二进制 右移一位（二进制右移运算符。左操作数的值向右移动右操作数指定的位数）  
 * 例如 A>> 2将得出15，即0000 1111  当A=60    111100  > 00111100  二进制右移   即00001111
 * 例如0001000010100001  A110 > 10A1 > 4257 > 0001000010100001右移一位000100001010000 截取为0100001010000 转成十进制2128
 */
int Rshiftbit(String hexStr) {
    int result = 0;
    int data_num =  (int)strtol(hexStr.c_str(), NULL, 16); // 二进制表示为 1010
        result = data_num >> 1;       // 结果为 0101，即 5
   return result;  
}
