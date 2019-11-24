package bus

type Bus struct {
	Subs map[string][]chan interface{}
}

func New() *Bus {
	rV := Bus{}
	rV.Subs = make(map[string][]chan interface{})
	return &rV
}

// Pub broadcasts a message to any current subscribers.
func (b *Bus) Pub(subject string, value interface{}) error {
	subs := b.Subs[subject]
	for _, target := range subs {
		target <- value
	}
	return nil
}

// Sub shouldn't be called a lot or in a loop, but provides a subscription to
// arbitrary subject names. Exact matches only. You must NOT alter the value
// in any way. There is no cancellation method at present.
func (b *Bus) Sub(subject string, cb func(value interface{})) error {
	c := make(chan interface{}, 10)
	b.Subs[subject] = append(b.Subs[subject], c)

	go func() {
		for {
			cb(<-c)
		}
	}()

	return nil
}
