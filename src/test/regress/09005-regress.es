
public class Btest 
{

	public var btest: Btest = this;

	private var 	txtPassFail: String;
	private var 	txtResults: String;


	var scriptCounter: num = 1;
	var totalTests: num = 9;
	var results:string = "";
	var desc:string = "";
	var tpass: num = 0;
	var tfail: num = 0;
	
	var a: num = 0;
	var b: num = 0;
	var c: num=0;
	var LastName;
	var Soc_Sec_No;
	var is_morning;
	var is_evening;
	var FirstString;
	var SecondString;
	var z;
	var s;
	var s16;
	
	public function loadtest(){
		for(scriptCounter=1;scriptCounter <= totalTests; scriptCounter++){
			setResult("\n-------------");
			processTest(scriptCounter);
		}
		txtPassFail = "Pass: " + tpass + " Failed: " + tfail;
	}
	
	public function processTest(counter){
		if(counter==1){
			desc="Declaring Variables";
			setResult("\nTest #: " + counter);
			setResult("\nDesc: " + desc);
			
			LastName="Pyanin";
			Soc_Sec_No=123456789;
			is_morning=true;
			is_evening=false;					
			
			assert(LastName=="Pyanin");
			assert(Soc_Sec_No==123456789);
			assert(is_morning==true);
			assert(is_evening==false);
			
			myAssert(LastName,"Pyanin",counter);
			myAssert(Soc_Sec_No,1234567899,counter);
			myAssert(is_morning,true,counter);
			myAssert(is_evening,false,counter);
		
		} else if(counter==2){
			desc="Assigning Variables to Variables";
			setResult("\nTest #: " + counter);
			setResult("\nDesc: " + desc);

			FirstString = "Hello World";
			SecondString = FirstString;
		
			assert(FirstString == "Hello World");
			assert(SecondString == FirstString);
							
			myAssert(FirstString,"Hello World",counter);
			myAssert(SecondString,FirstString,counter);
			
			FirstString = "Goodbye World";
			
			assert(FirstString == "Goodbye World");
			myAssert(FirstString,"Goodbye World",counter);
			
		} else if(counter==3){
			varReset();
			desc="Adding";
			setResult("\nTest #: " + counter);
			setResult("\nDesc: " + desc);
			
			a=a+10;
			b+=53;
						
			assert(a == 10);
			assert(b == 53);
							
			myAssert(a,10,counter);
			myAssert(b,53,counter);
			
			b+=a;
			
			assert(b == 63);
			myAssert(b,63,counter);
			
		} else if(counter==4){
			varReset();
			desc="Sub";
			setResult("\nTest #: " + counter);
			setResult("\nDesc: " + desc);
			
			a=a-8;
			b+=24;
						
			assert(a == -8);
			assert(b == 24);
							
			myAssert(a,-8,counter);
			myAssert(b,24,counter);
			
			b=b-a;
			assert(b == 32);
			myAssert(b,32,counter);
			
		} else if(counter==5){
			varReset();
			desc="Mul";
			setResult("\nTest #: " + counter);
			setResult("\nDesc: " + desc);
			
			a=8;
			a=a * 8;
			
			b=4;
			b*=24;
						
			assert(a == 64);
			assert(b == 96);
							
			myAssert(a,64,counter);
			myAssert(b,96,counter);
			
			b=b*a;
			assert(b == 6144);
			myAssert(b,6144,counter);
			
		} else if(counter==6){
			varReset();
			desc="Div";
			setResult("\nTest #: " + counter);
			setResult("\nDesc: " + desc);
			
			a=60;
			a=a / 15;
			
			b=48;
			b/=4;
						
			assert(a == 4);
			assert(b == 12);
							
			myAssert(a,4,counter);
			myAssert(b,12,counter);
			
			b=b/a;
			assert(b == 3);
			myAssert(b,3,counter);
			
		} else if(counter==7){
			varReset();
			desc="BZ 374";
			setResult("\nTest #: " + counter);
			setResult("\nDesc: " + desc);
			
			z=4
			s=z-15;
						
			assert(z == 4);
			assert(s == -11);
							
			myAssert(z,4,counter);
			myAssert(s,-11,counter);
			
		} else if(counter==8){
/*				varReset();
			desc="BZ 380";
			setResult("\nTest #: " + counter);
			setResult("\nDesc: " + desc);
			
			s16=20-15;
						
			assert(s16 == 5);
			myAssert(s16,5,counter);
*/
		}else if(counter==9){
			varReset();
			desc="series";
			setResult("\nTest #: " + counter);
			setResult("\nDesc: " + desc);
			
			a=5;
			b=14;
			c=5432;
			z=a-b*c*(a+b)/a+b;
			print(z);			
			assert(z == -288963);
			myAssert(z,-288963,counter);
		}	
	}
	
	public function setResult(result)
	{
		txtResults = txtResults + result;
	}
	
	public function myAssert(actual,expected,counter)
	{
		if (actual==expected){
			setResult("\nExpected: " + expected);
			setResult("\nActual: " + actual);
			setResult("\n--Test #: " + counter + "-pass");
			print("\n--Test #: " + counter + "-pass");
			tpass+=1;
		}else{
			setResult("\nExpected: " + expected);
			setResult("\nActual: " + actual);
			setResult("\n--Test #: " + counter + "-fail");
			print("\n--Test #:-fail");
			tfail+=1;
		}	
	}
	
	public function varReset(){
			a= 0;
		b = 0;
		c= 0;
		z=0;
		s=0;
		s16=0;
	}
}
