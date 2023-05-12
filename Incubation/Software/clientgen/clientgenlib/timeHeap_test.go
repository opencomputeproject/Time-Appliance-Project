/*
Copyright (c) Facebook, Inc. and its affiliates.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package clientgenlib

import (
	"testing"
	"time"
	"math/rand"

	"github.com/stretchr/testify/require"
)

func Test_isEmptyAfterSoleElementRemoved(t *testing.T) {
	var heap timeHeap
	var itemNow timeItem
	heap.Init(0)
	itemNow.priority = time.Now()
	itemNow.index = -1

	heap.SafePush(&itemNow)
	require.Equal(t, 1, heap.Len())

	heap.SafePop()
	require.Equal(t, 0, heap.Len())
	require.Nil(t, heap.Peek())

}

func Test_peekSingleItem(t *testing.T) {
	var heap timeHeap
	var itemNow timeItem
	heap.Init(0)
	itemNow.priority = time.Now()
	itemNow.index = -1

	heap.SafePush(&itemNow)

	require.Equal(t, &itemNow, heap.SafePeek().(*timeItem))
	require.Equal(t, true, heap.Len() > 0)
}

func Test_returnsTwoOrderedItems(t *testing.T) {
	var heap timeHeap
	var itemNow timeItem
	var itemLater timeItem

	heap.Init(0)
	itemNow.priority = time.Now()
	itemNow.index = -1
	itemLater.priority = time.Now()
	itemLater.index = -1

	heap.SafePush(&itemNow)
	heap.SafePush(&itemLater)

	require.Equal(t, &itemNow, heap.SafePeek().(*timeItem))
	require.Equal(t, true, heap.Len() == 2)
	require.Equal(t, &itemNow, heap.SafePop().(*timeItem))

	require.Equal(t, &itemLater, heap.SafePeek().(*timeItem))
	require.Equal(t, true, heap.Len() == 1)
	require.Equal(t, &itemLater, heap.SafePop().(*timeItem))
}

func Test_multipleInOrderItems(t *testing.T) {
	var heap timeHeap
	heap.Init(0)

	var items []timeItem
	items = make([]timeItem, 100)

	for i := 0; i < 100; i++ {
		items[i].index = -1
		items[i].priority = time.Now()
		heap.SafePush(&items[i])
	}
	for i := 0; i < 100; i++ {
		require.Equal(t, &items[i], heap.SafePeek().(*timeItem))
		require.Equal(t, true, heap.Len() == (100-i))
		require.Equal(t, &items[i], heap.SafePop().(*timeItem))
	}
}

func Test_randomReorderItems(t *testing.T) {
	var heap timeHeap
	heap.Init(0)

	var items []timeItem
	items = make([]timeItem, 100)

	for i := 0; i < 100; i++ {
		items[i].index = -1
		items[i].priority = time.Now()
		heap.SafePush(&items[i])
	}
	rand.Seed(86)
	// update random items for a while
	for i := 0; i < 100; i++ {
		index := rand.Intn(100)
		heap.SafeUpdate(&items[index], nil, time.Now())
	}
	// verify things pop in order
	for heap.Len() > 0 {
		minItem := heap.SafePop().(*timeItem)
		nextMinItem := heap.SafePop().(*timeItem)
		if nextMinItem != nil {
			// fail if what I popped is after what is now peeked
			if minItem.priority.After(nextMinItem.priority) {
				require.Equal(t, true, false)
			}
		}
	}
}

func Test_randomRemove(t *testing.T) {
	var heap timeHeap
	heap.Init(0)

	var items []timeItem
	items = make([]timeItem, 100)

	for i := 0; i < 100; i++ {
		items[i].index = -1
		items[i].priority = time.Now()
		heap.SafePush(&items[i])
	}
	rand.Seed(86)
	// update random items for a while
	for i := 0; i < 20; i++ {
		index := rand.Intn(100)
		heap.SafeRemove(&items[index])
	}
	// verify things pop in order
	for heap.Len() > 0 {
		minItem := heap.SafePop().(*timeItem)
		nextMinItem := heap.SafePop().(*timeItem)
		if nextMinItem != nil {
			// fail if what I popped is after what is now peeked
			if minItem.priority.After(nextMinItem.priority) {
				require.Equal(t, true, false)
			}
		}
	}
}

func Test_randomOperations(t *testing.T) {
	var heap timeHeap
	heap.Init(0)

	var items []timeItem
	items = make([]timeItem, 1000)

	for i := 0; i < 1000; i++ {
		items[i].index = -1
		items[i].priority = time.Now()
		heap.SafePush(&items[i])
	}
	rand.Seed(86)

	// do random things to items
	for i := 0; i < 100; i++ {
		operation := rand.Intn(3)
		if operation == 0 { // pop
			heap.SafePop()
		} else if operation == 1 { // remove a random item
			index := rand.Intn(heap.Len())
			heap.SafeRemove(&items[index])
		} else if operation == 2 { // update a random item
			index := rand.Intn(heap.Len())
			heap.SafeUpdate(&items[index], nil, time.Now())
		}
	}

	// verify items pop in order
	for heap.Len() > 0 {
		minItem := heap.SafePop().(*timeItem)
		nextMinItem := heap.SafePop().(*timeItem)
		if nextMinItem != nil {
			// fail if what I popped is after what is now peeked
			if minItem.priority.After(nextMinItem.priority) {
				require.Equal(t, true, false)
			}
		}
	}

}
