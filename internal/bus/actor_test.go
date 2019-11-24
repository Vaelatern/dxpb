package bus

import (
	"testing"
)

type testObj struct {
	val int
}

func TestSimpleCase(t *testing.T) {
	var end chan interface{}
	end = make(chan interface{})

	a := New()
	err := a.Pub("abc", testObj{val: 1})
	if err != nil {
		t.Errorf("Pub without any subs errors")
	}

	err = a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
			t.Errorf("Got wrong value")
		}
		end <- 1
	})

	err = a.Pub("abc", testObj{val: 2})

	if err != nil {
		t.Errorf("Pub with a sub errors")
	}
	_ = <-end
}

func TestFourSubs(t *testing.T) {
	var end chan interface{}
	end = make(chan interface{})

	a := New()
	err := a.Pub("abc", testObj{val: 1})
	if err != nil {
		t.Errorf("Pub without any subs errors")
	}

	err = a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
			t.Errorf("Got wrong value")
		}
		end <- 1
	})
	err = a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
			t.Errorf("Got wrong value")
		}
		end <- 2
	})
	err = a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
			t.Errorf("Got wrong value")
		}
		end <- 3
	})
	err = a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
			t.Errorf("Got wrong value")
		}
		end <- 4
	})

	err = a.Pub("abc", testObj{val: 2})

	if err != nil {
		t.Errorf("Pub with a sub errors")
	}
	_ = <-end
	_ = <-end
	_ = <-end
	_ = <-end
}

func BenchmarkOneSub(b *testing.B) {
	var end chan interface{}
	end = make(chan interface{}, b.N/100)
	a := New()

	err := a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
		}
		end <- nil
	})

	for i := 0; i < b.N; i++ {
		err = a.Pub("abc", testObj{val: 2})
		if err != nil {
		}
		<-end
	}
}

func BenchmarkTwoSub(b *testing.B) {
	var end chan interface{}
	end = make(chan interface{}, b.N/100)
	a := New()

	err := a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
		}
		end <- nil
	})
	err = a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
		}
		end <- nil
	})

	for i := 0; i < b.N; i++ {
		err = a.Pub("abc", testObj{val: 2})
		if err != nil {
		}
		<-end
		<-end
	}
}

func BenchmarkFourSub(b *testing.B) {
	var end chan interface{}
	end = make(chan interface{}, b.N/100)
	a := New()

	err := a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
		}
		end <- nil
	})
	err = a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
		}
		end <- nil
	})
	err = a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
		}
		end <- nil
	})
	err = a.Sub("abc", func(v interface{}) {
		if v.(testObj).val != 2 {
		}
		end <- nil
	})

	for i := 0; i < b.N; i++ {
		err = a.Pub("abc", testObj{val: 2})
		if err != nil {
		}
		<-end
		<-end
		<-end
		<-end
	}
}
